#include "ControlECU.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

// Misma relación que usó Alan en el simulador, rpm = 800 + 22 * velocidad + 18 * acelerador
constexpr double IDLE_RPM = 800.0;
constexpr double RPM_PER_KMH = 22.0;
constexpr double RPM_PER_THROTTLE = 18.0;
constexpr double SPEED_TOLERANCE = 15.0;

ControlECU::ControlECU(const std::vector<Sensor> &gateway_sensors) : currentState(ECUState::INIT),
                                                                     controlData{
                                                                         {getSensor(gateway_sensors, SignalId::SPEED),
                                                                          getSensor(gateway_sensors, SignalId::RPM),
                                                                          getSensor(gateway_sensors, SignalId::TEMPERATURE),
                                                                          getSensor(gateway_sensors, SignalId::BATTERY_VOLTAGE),
                                                                          getSensor(gateway_sensors, SignalId::THROTTLE),
                                                                          getSensor(gateway_sensors, SignalId::OIL_PRESSURE)},
                                                                         currentState,
                                                                         cycleCount}
{
}

// El orden importa: isIncoherent solo se evalúa cuando isDegraded ya confirmó datos válidos y frescos
void ControlECU::runControlCycle()
{
    ++cycleCount;
    switch (currentState)
    {
    case ECUState::INIT:
        if (hasCriticalFault())
        {
            stateTransition(ECUState::SAFE_STATE);
            break;
        }
        if (isDegraded())
        {
            stateTransition(ECUState::DEGRADED);
            break;
        }
        if (isIncoherent())
        {
            stateTransition(ECUState::SAFE_STATE);
            break;
        }
        stateTransition(ECUState::OPERATIONAL);
        break;
    case ECUState::OPERATIONAL:
        if (hasCriticalFault())
        {
            stateTransition(ECUState::SAFE_STATE);
            break;
        }
        if (isDegraded())
        {
            stateTransition(ECUState::DEGRADED);
            break;
        }
        if (isIncoherent())
        {
            stateTransition(ECUState::SAFE_STATE);
        }
        break;
    case ECUState::DEGRADED:
        if (hasCriticalFault())
        {
            stateTransition(ECUState::SAFE_STATE);
            break;
        }
        if (isDegraded())
        {
            break;
        }
        if (isIncoherent())
        {
            stateTransition(ECUState::SAFE_STATE);
            break;
        }
        stateTransition(ECUState::OPERATIONAL);
        break;
    case ECUState::SAFE_STATE:
        break;
    }
}

bool ControlECU::hasCriticalFault()
{
    if (controlData.sensors.temperature.getState() == SignalState::OUT_OF_RANGE)
    {
        return true;
    }

    if (controlData.sensors.batteryVoltage.getState() == SignalState::OUT_OF_RANGE)
    {
        return true;
    }

    if (controlData.sensors.temperature.getState() == SignalState::NOT_AVAILABLE ||
        controlData.sensors.batteryVoltage.getState() == SignalState::NOT_AVAILABLE)
    {
        return true;
    }

    return false;
}

bool ControlECU::isDegraded()
{
    if (controlData.sensors.rpm.getState() != SignalState::VALID ||
        controlData.sensors.oilPressure.getState() != SignalState::VALID ||
        controlData.sensors.speed.getState() != SignalState::VALID ||
        controlData.sensors.throttle.getState() != SignalState::VALID)
    {
        return true;
    }

    // Un sensor sigue VALID con su valor anterior mientras no supere sus ciclos tolerados
    if (controlData.sensors.speed.getMissedCycles() > 0 ||
        controlData.sensors.rpm.getMissedCycles() > 0 ||
        controlData.sensors.throttle.getMissedCycles() > 0)
    {
        return true;
    }

    if (controlData.sensors.throttle.getValue() > 80 && controlData.sensors.rpm.getValue() < 500)
    {
        return true;
    }

    return false;
}

bool ControlECU::isIncoherent()
{
    double expectedSpeed = (controlData.sensors.rpm.getValue() - IDLE_RPM -
                            RPM_PER_THROTTLE * controlData.sensors.throttle.getValue()) /
                           RPM_PER_KMH;
    return std::abs(controlData.sensors.speed.getValue() - expectedSpeed) > SPEED_TOLERANCE;
}

void ControlECU::stateTransition(ECUState newState)
{
    currentState = newState;
}

const ECUData &ControlECU::getControlData() const
{
    return controlData;
}

const Sensor &ControlECU::getSensor(
    const std::vector<Sensor> &sensors,
    SignalId id)
{
    for (const Sensor &sensor : sensors)
    {
        if (sensor.getId() == id)
        {
            return sensor;
        }
    }
    throw std::out_of_range("ControlECU::getSensor: la Gateway no tiene un sensor con ese ID");
}

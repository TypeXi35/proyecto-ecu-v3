#include "ControlECU.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

// Misma relación que usó Alan en el simulador, rpm = 800 + 22 * velocidad + 18 * acelerador
constexpr double IDLE_RPM = 800.0;
constexpr double RPM_PER_KMH = 22.0;
constexpr double RPM_PER_THROTTLE = 18.0;
constexpr double SPEED_TOLERANCE = 15.0;

// Válida y actualizada en cada ciclo
static bool isFresh(const Sensor &sensor)
{
    return sensor.getState() == SignalState::VALID && sensor.getMissedCycles() == 0;
}

ControlECU::ControlECU(const std::vector<Sensor> &gateway_sensors) : currentState(ECUState::INIT),
                                                                     data{
                                                                         {getSensor(gateway_sensors, SignalId::SPEED),
                                                                          getSensor(gateway_sensors, SignalId::RPM),
                                                                          getSensor(gateway_sensors, SignalId::TEMPERATURE),
                                                                          getSensor(gateway_sensors, SignalId::BATTERY_VOLTAGE),
                                                                          getSensor(gateway_sensors, SignalId::THROTTLE),
                                                                          getSensor(gateway_sensors, SignalId::OIL_PRESSURE)},
                                                                         currentState}
{
}

void ControlECU::runControlCycle()
{
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
        }
        break;
    case ECUState::DEGRADED:
        if (hasCriticalFault())
        {
            stateTransition(ECUState::SAFE_STATE);
            break;
        }
        if (isDegraded() == false)
        {
            stateTransition(ECUState::OPERATIONAL);
        }
        break;
    case ECUState::SAFE_STATE:
        break;
    }
}

bool ControlECU::hasCriticalFault()
{
    if (data.sensors.temperature.getState() == SignalState::OUT_OF_RANGE)
    {
        return true;
    }

    if (data.sensors.batteryVoltage.getState() == SignalState::OUT_OF_RANGE)
    {
        return true;
    }

    if (data.sensors.temperature.getState() == SignalState::NOT_AVAILABLE ||
        data.sensors.batteryVoltage.getState() == SignalState::NOT_AVAILABLE)
    {
        return true;
    }

    // Solo se compara con datos válidos de cada ciclo
    if (isFresh(data.sensors.speed) && isFresh(data.sensors.rpm) && isFresh(data.sensors.throttle))
    {
        double expectedSpeed = (data.sensors.rpm.getValue() - IDLE_RPM -
                                RPM_PER_THROTTLE * data.sensors.throttle.getValue()) /
                               RPM_PER_KMH;
        bool coherent = std::abs(data.sensors.speed.getValue() - expectedSpeed) <= SPEED_TOLERANCE;
        if (!coherent)
        {
            return true;
        }
    }
    return false;
}

bool ControlECU::isDegraded()
{
    if (data.sensors.rpm.getState() != SignalState::VALID ||
        data.sensors.oilPressure.getState() != SignalState::VALID ||
        data.sensors.speed.getState() != SignalState::VALID ||
        data.sensors.throttle.getState() != SignalState::VALID)
    {
        return true;
    }

    if (data.sensors.throttle.getValue() > 80 && data.sensors.rpm.getValue() < 500)
    {
        return true;
    }

    return false;
}

void ControlECU::stateTransition(ECUState newState)
{
    currentState = newState;
}

ECUData ControlECU::getControlData()
{
    return data;
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

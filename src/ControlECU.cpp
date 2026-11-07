#include "ControlECU.hpp"

#include <cmath>
#include <algorithm>
#include "MonitoredSensor.hpp"
#include "OperationalLimits.hpp"

ControlECU::ControlECU() : currentState(ECUState::INIT),
                           controlData{
                               {},
                               currentState,
                               cycleCount,
                               FaultCode::NONE},
                           monitoredSensors{{{controlData.sensors.temperature, TEMPERATURE_OPERATING_MIN_C, TEMPERATURE_OPERATING_MAX_C, Criticality::CRITICAL},
                                             {controlData.sensors.oilPressure, OIL_PRESSURE_OPERATING_MIN_BAR, OIL_PRESSURE_OPERATING_MAX_BAR, Criticality::CRITICAL},
                                             {controlData.sensors.speed, SPEED_OPERATING_MIN_KMH, SPEED_OPERATING_MAX_KMH, Criticality::NON_CRITICAL},
                                             {controlData.sensors.rpm, RPM_OPERATING_MIN, RPM_OPERATING_MAX, Criticality::NON_CRITICAL},
                                             {controlData.sensors.throttle, THROTTLE_OPERATING_MIN_PCT, THROTTLE_OPERATING_MAX_PCT, Criticality::NON_CRITICAL},
                                             {controlData.sensors.batteryVoltage, BATTERY_OPERATING_MIN_V, BATTERY_OPERATING_MAX_V, Criticality::NON_CRITICAL}}}
{
}

void ControlECU::loadSensors(const std::vector<Sensor> &sensors)
{
    controlData.sensors = ControlSensors{};
    std::for_each(sensors.begin(), sensors.end(),
        [this](const Sensor &sensor) {
            switch (sensor.getId())
            {
            case SignalId::SPEED:
                controlData.sensors.speed = sensor;
                break;
            case SignalId::RPM:
                controlData.sensors.rpm = sensor;
                break;
            case SignalId::TEMPERATURE:
                controlData.sensors.temperature = sensor;
                break;
            case SignalId::BATTERY_VOLTAGE:
                controlData.sensors.batteryVoltage = sensor;
                break;
            case SignalId::THROTTLE:
                controlData.sensors.throttle = sensor;
                break;
            case SignalId::OIL_PRESSURE:
                controlData.sensors.oilPressure = sensor;
                break;
            default:
                break;
            }
        });
}

void ControlECU::runControlCycle(const std::vector<Sensor> &sensors)
{
    ++cycleCount;
    loadSensors(sensors);
    switch (currentState)
    {
    case ECUState::INIT:
        stateTransition(ECUState::SELF_TEST);
        break;
    case ECUState::SELF_TEST:
        stateTransition(generalHealthCheck());
        break;
    case ECUState::OPERATIONAL:
        stateTransition(generalHealthCheck());
        break;
    case ECUState::DEGRADED:
        stateTransition(generalHealthCheck());
        break;
    case ECUState::SAFE_STATE:
        stateTransition(ECUState::SHUTDOWN);
        break;
    case ECUState::SHUTDOWN:
        break;
    default:
        stateTransition(ECUState::SAFE_STATE);
        break;
    }
}

bool ControlECU::isUnhealthy(const Sensor &sensor)
{
    return sensor.getState() != SignalState::VALID || sensor.getMissedCycles() > 0;
}

ECUState ControlECU::generalHealthCheck() const
{
    if (hasCriticalFault())
    {
        return ECUState::SAFE_STATE;
    }
    if (isDegraded())
    {
        return ECUState::DEGRADED;
    }
    if (isIncoherent())
    {
        return ECUState::SAFE_STATE;
    }
    return ECUState::OPERATIONAL;
}

bool ControlECU::isOutOfLimits(const MonitoredSensor &monitored)
{
    const auto value = monitored.sensor.getValue();
    return value < monitored.operatingMin || value > monitored.operatingMax;
}

bool ControlECU::hasCriticalFault() const
{
    return std::any_of(monitoredSensors.begin(), monitoredSensors.end(),
                       [](const MonitoredSensor &monitored)
                       {
                           return monitored.criticality == Criticality::CRITICAL && (monitored.sensor.getState() != SignalState::VALID ||
                                                                                     isOutOfLimits(monitored));
                       });
}

bool ControlECU::isDegraded() const
{
    const bool nonCriticalProblem = std::any_of(monitoredSensors.begin(), monitoredSensors.end(),
                                                [](const MonitoredSensor &monitored)
                                                {
                                                    return monitored.criticality == Criticality::NON_CRITICAL && (isUnhealthy(monitored.sensor) || isOutOfLimits(monitored));
                                                });
    const bool criticalStale = std::any_of(monitoredSensors.begin(), monitoredSensors.end(),
                                           [](const MonitoredSensor &monitored)
                                           {
                                               return monitored.criticality == Criticality::CRITICAL && monitored.sensor.getMissedCycles() > 0;
                                           });
    return nonCriticalProblem || criticalStale;
}

bool ControlECU::isIncoherent() const
{
    if (controlData.sensors.throttle.getValue() > HIGH_THROTTLE && controlData.sensors.rpm.getValue() < MIN_RPM_UNDER_LOAD)
    {
        return true;
    }
    double expectedSpeed = (controlData.sensors.rpm.getValue() - IDLE_RPM -
                            RPM_PER_THROTTLE * controlData.sensors.throttle.getValue()) /
                           RPM_PER_KMH;
    return std::abs(controlData.sensors.speed.getValue() - expectedSpeed) > SPEED_TOLERANCE;
}

void ControlECU::stateTransition(ECUState nextState)
{
    currentState = nextState;
}

const ECUData &ControlECU::getControlData() const
{
    return controlData;
}

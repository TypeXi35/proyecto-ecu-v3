#include "ControlECU.hpp"

#include <cmath>
#include <algorithm>
#include "MonitoredSensor.hpp"
#include "OperationalLimits.hpp"

ControlECU::ControlECU() : controlData{
                               {},
                               ECUState::INIT,
                               0,
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
    loadSensors(sensors);
    switch (controlData.currentState)
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
    ++controlData.cycleCount;
}

bool ControlECU::isUnhealthy(const Sensor &sensor)
{
    return sensor.getState() != SignalState::VALID || sensor.getMissedCycles() > 0;
}

ECUState ControlECU::generalHealthCheck()
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
    controlData.faultCode = FaultCode::NONE;
    return ECUState::OPERATIONAL;
}

bool ControlECU::isOutOfLimits(const MonitoredSensor &monitored)
{
    const auto value = monitored.sensor.getValue();
    return value < monitored.operatingMin || value > monitored.operatingMax;
}

bool ControlECU::hasCriticalFault()
{
    return std::any_of(monitoredSensors.begin(), monitoredSensors.end(),
                       [this](const MonitoredSensor &monitored)
                       {
                           if (monitored.criticality != Criticality::CRITICAL)
                           {
                               return false;
                           }
                           if (monitored.sensor.getState() == SignalState::NOT_AVAILABLE)
                           {
                               controlData.faultCode = FaultCode::CRITICAL_SENSOR_LOST;
                               return true;
                           }
                           if (monitored.sensor.getState() != SignalState::VALID || isOutOfLimits(monitored))
                           {
                               controlData.faultCode = FaultCode::CRITICAL_OUT_OF_LIMITS;
                               return true;
                           }
                           return false;
                       });
}

bool ControlECU::isDegraded()
{
    const bool nonCriticalProblem = std::any_of(monitoredSensors.begin(), monitoredSensors.end(),
                                                [this](const MonitoredSensor &monitored)
                                                {
                                                    if (monitored.criticality != Criticality::NON_CRITICAL)
                                                    {
                                                        return false;
                                                    }
                                                    if (monitored.sensor.getState() == SignalState::OUT_OF_RANGE || isOutOfLimits(monitored))
                                                    {
                                                        controlData.faultCode = FaultCode::SIGNAL_OUT_OF_LIMITS;
                                                        return true;
                                                    }
                                                    if (isUnhealthy(monitored.sensor))
                                                    {
                                                        controlData.faultCode = FaultCode::SENSOR_DEGRADED;
                                                        return true;
                                                    }
                                                    return false;
                                                });
    const bool criticalStale = std::any_of(monitoredSensors.begin(), monitoredSensors.end(),
                                           [this](const MonitoredSensor &monitored)
                                           {
                                               if (monitored.criticality == Criticality::CRITICAL && monitored.sensor.getMissedCycles() > 0)
                                               {
                                                   controlData.faultCode = FaultCode::SENSOR_DEGRADED;
                                                   return true;
                                               }
                                               return false;
                                           });
    return nonCriticalProblem || criticalStale;
}

bool ControlECU::isIncoherent()
{
    if (controlData.sensors.throttle.getValue() > HIGH_THROTTLE && controlData.sensors.rpm.getValue() < MIN_RPM_UNDER_LOAD)
    {
        controlData.faultCode = FaultCode::INCOHERENT_SIGNALS;
        return true;
    }
    double expectedSpeed = (controlData.sensors.rpm.getValue() - IDLE_RPM -
                            RPM_PER_THROTTLE * controlData.sensors.throttle.getValue()) /
                           RPM_PER_KMH;
    if (std::abs(controlData.sensors.speed.getValue() - expectedSpeed) > SPEED_TOLERANCE)
    {
        controlData.faultCode = FaultCode::INCOHERENT_SIGNALS;
        return true;
    }
    return false;
}

void ControlECU::stateTransition(ECUState nextState)
{
    controlData.currentState = nextState;
}

const ECUData &ControlECU::getControlData() const
{
    return controlData;
}
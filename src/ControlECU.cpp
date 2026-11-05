#include "ControlECU.hpp"

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include "GatewayECU.hpp"
#include "MonitoredSensor.hpp"

// Misma relación que usó Alan en el simulador, rpm = 800 + 22 * velocidad + 18 * acelerador
constexpr double IDLE_RPM = 800.0;
constexpr double RPM_PER_KMH = 22.0;
constexpr double RPM_PER_THROTTLE = 18.0;
constexpr double SPEED_TOLERANCE = 15.0;
constexpr double HIGH_THROTTLE = 80.0;
constexpr double MIN_RPM_UNDER_LOAD = 500.0;

constexpr double SPEED_OPERATING_MIN_KMH = 0.0;
constexpr double SPEED_OPERATING_MAX_KMH = 200.0;
constexpr double RPM_OPERATING_MIN = 0.0; // no absolute minimum: low RPM is judged in context by isIncoherent()
constexpr double RPM_OPERATING_MAX = 6800.0;
constexpr double TEMPERATURE_OPERATING_MIN_C = -30.0;
constexpr double TEMPERATURE_OPERATING_MAX_C = 110.0;
constexpr double THROTTLE_OPERATING_MIN_PCT = 0.0;
constexpr double THROTTLE_OPERATING_MAX_PCT = 100.0;
constexpr double BATTERY_OPERATING_MIN_V = 11.5;
constexpr double BATTERY_OPERATING_MAX_V = 15.0;
constexpr double OIL_PRESSURE_OPERATING_MIN_BAR = 0.8;
constexpr double OIL_PRESSURE_OPERATING_MAX_BAR = 6.0;

ControlECU::ControlECU(const GatewayECU &gateway) : currentState(ECUState::INIT),
                                                    controlData{
                                                        {gateway.findSensor(SignalId::SPEED),
                                                         gateway.findSensor(SignalId::RPM),
                                                         gateway.findSensor(SignalId::TEMPERATURE),
                                                         gateway.findSensor(SignalId::BATTERY_VOLTAGE),
                                                         gateway.findSensor(SignalId::THROTTLE),
                                                         gateway.findSensor(SignalId::OIL_PRESSURE)},
                                                        currentState,
                                                        cycleCount,
                                                        faultCode
                                                    },
                                                    monitoredSensors{{
                                                        {controlData.sensors.temperature, TEMPERATURE_OPERATING_MIN_C, TEMPERATURE_OPERATING_MAX_C, Criticality::CRITICAL},
                                                        {controlData.sensors.oilPressure, OIL_PRESSURE_OPERATING_MIN_BAR, OIL_PRESSURE_OPERATING_MAX_BAR, Criticality::CRITICAL},
                                                        {controlData.sensors.speed, SPEED_OPERATING_MIN_KMH, SPEED_OPERATING_MAX_KMH, Criticality::NON_CRITICAL},
                                                        {controlData.sensors.rpm, RPM_OPERATING_MIN, RPM_OPERATING_MAX, Criticality::NON_CRITICAL},
                                                        {controlData.sensors.throttle, THROTTLE_OPERATING_MIN_PCT, THROTTLE_OPERATING_MAX_PCT, Criticality::NON_CRITICAL},
                                                        {controlData.sensors.batteryVoltage, BATTERY_OPERATING_MIN_V, BATTERY_OPERATING_MAX_V, Criticality::NON_CRITICAL}}}
{
}

void ControlECU::runControlCycle()
{
    ++cycleCount;
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
                [](const MonitoredSensor &monitored){
                    return monitored.criticality == Criticality::CRITICAL && (monitored.sensor.getState() != SignalState::VALID ||
                        isOutOfLimits(monitored));
                });
}

bool ControlECU::isDegraded() const
{
    const bool nonCriticalProblem = std::any_of(monitoredSensors.begin(), monitoredSensors.end(),
        [](const MonitoredSensor &monitored){
            return monitored.criticality == Criticality::NON_CRITICAL && (isUnhealthy(monitored.sensor) || isOutOfLimits(monitored));
        });
    const bool criticalStale = std::any_of(monitoredSensors.begin(), monitoredSensors.end(),
            [](const MonitoredSensor &monitored){
                return  monitored.criticality == Criticality::CRITICAL && monitored.sensor.getMissedCycles() > 0;
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

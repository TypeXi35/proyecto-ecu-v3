#pragma once

#include "ControlData.hpp"
#include "Sensor.hpp"
#include "ECUState.hpp"
#include "SignalTypes.hpp"
#include "GatewayECU.hpp"
#include "MonitoredSensor.hpp"

#include <array>

class ControlECU
{
private:
    ECUState currentState;
    unsigned int cycleCount{0};
    ECUData controlData;
    FaultCode faultCode;
    std::array<const MonitoredSensor, 6> monitoredSensors;
    
public:
    explicit ControlECU(const GatewayECU &gateway);
    void runControlCycle();
    const ECUData &getControlData() const;

private:
    ECUState generalHealthCheck() const;
    static bool isUnhealthy(const Sensor &sensor);
    static bool isOutOfLimits(const MonitoredSensor &monitored);
    bool hasCriticalFault() const;
    bool isDegraded() const;
    bool isIncoherent() const;
    void stateTransition(ECUState nextState);
};

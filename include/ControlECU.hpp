#pragma once

#include "ControlData.hpp"
#include "Sensor.hpp"
#include "ECUState.hpp"
#include "SignalTypes.hpp"
#include "GatewayECU.hpp"
#include "MonitoredSignal.hpp"

#include <vector>
#include <array>

class ControlECU
{
private:
    ECUState currentState;
    unsigned int cycleCount{0};
    ECUData controlData;
    ECUState newState;
    FaultCode faultCode;
    std::array<const MonitoredSignal, 2> criticalSensors;
    std::array<const MonitoredSignal, 4> nonCriticalSensors;

public:
    explicit ControlECU(const GatewayECU &gateway);
    void runControlCycle();
    const ECUData &getControlData() const;

private:
    ECUState generalHealthCheck() const;
    static bool isUnhealthy(const Sensor &sensor);
    static bool isOutOfLimits(const MonitoredSignal &signal);
    bool hasCriticalFault() const;
    bool isDegraded() const;
    bool isIncoherent() const;
    void stateTransition(ECUState nextState);
};

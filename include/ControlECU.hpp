#pragma once

#include "ControlData.hpp"
#include "Sensor.hpp"
#include "ECUState.hpp"
#include "SignalTypes.hpp"
#include "MonitoredSensor.hpp"

#include <vector>
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
    ControlECU();
    void runControlCycle(const std::vector<Sensor> &sensors);
    const ECUData &getControlData() const;

private:
    void loadSensors(const std::vector<Sensor> &sensors);
    ECUState generalHealthCheck() const;
    static bool isUnhealthy(const Sensor &sensor);
    static bool isOutOfLimits(const MonitoredSensor &monitored);
    bool hasCriticalFault() const;
    bool isDegraded() const;
    bool isIncoherent() const;
    void stateTransition(ECUState nextState);
};

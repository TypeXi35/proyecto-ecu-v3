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
    ECUData controlData;
    std::array<const MonitoredSensor, 6> monitoredSensors;
    
public:
    ControlECU();
    void runControlCycle(const std::vector<Sensor> &sensors);
    const ECUData &getControlData() const;
    
private:
    void controlDataUpdate();
    void loadSensors(const std::vector<Sensor> &sensors);
    ECUState generalHealthCheck();
    static bool isUnhealthy(const Sensor &sensor);
    static bool isOutOfLimits(const MonitoredSensor &monitored);
    bool hasCriticalFault();
    bool isDegraded();
    bool isIncoherent() const;
    void stateTransition(ECUState nextState);
};

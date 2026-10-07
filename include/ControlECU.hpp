#pragma once

#include "ControlData.hpp"
#include "Sensor.hpp"
#include "ECUState.hpp"
#include "SignalTypes.hpp"

#include <vector>

class ControlECU
{
private:
    ECUState currentState;
    ECUData data;

public:
    explicit ControlECU(const std::vector<Sensor> &gateway_sensors);
    void runControlCycle();
    ECUData getControlData();

private:
    bool hasCriticalFault();
    bool isDegraded();
    void stateTransition(ECUState newState);
    static const Sensor &getSensor(
        const std::vector<Sensor> &sensors,
        SignalId id);
};

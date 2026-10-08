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
    unsigned int cycleCount{0};
    ECUData controlData;

public:
    explicit ControlECU(const std::vector<Sensor> &gateway_sensors);
    void runControlCycle();
    const ECUData &getControlData() const;

private:
    bool hasCriticalFault();
    bool isDegraded();
    bool isIncoherent();
    void stateTransition(ECUState newState);
    static const Sensor &getSensor(
        const std::vector<Sensor> &sensors,
        SignalId id);
};

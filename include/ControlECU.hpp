#pragma once

#include <Sensor.hpp>
#include <ECUState.hpp>

#include <vector>

class ControlECU
{
private:
    std::vector<Sensor> &sensors;
    ECUState currentState;
    ECUState signalState;
    ECUState missingSignalsState;
    ECUState coherenceState;

public:
    ControlECU(std::vector<Sensor> &sensors);
    void runControlCycle();

private:
    ECUState checkSignalState();
    ECUState checkMissing();
    ECUState checkCoherence();
};

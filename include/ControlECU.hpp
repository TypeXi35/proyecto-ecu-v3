#pragma once

#include <Sensors.hpp>
#include <ECUState.hpp>

#include <vector>

class ControlECU{
    private:
        std::vector<Sensor>& sensors;
        MachineState currentState;
    public:
        ControlECU(std::vector<Sensor>& sensors);
    private:
        ECUState checkSignalState();
        ECUState checkMissing();
        ECUState checkCoherence();
};


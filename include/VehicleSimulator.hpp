#pragma once

#include "SignalTypes.hpp"

#include <random>
#include <vector>
#include <algorithm>

struct SignalFault
{
    SignalId id;
    FaultType type;
    unsigned int remainingCycles;
};

class VehicleSimulator
{
    private:

        std::mt19937 generator;

        std::normal_distribution<double> signalNoise;

        double speed;
        double rpm;
        double temperature;
        double batteryVoltage;
        double oilPressure;

        double throttle;

        // Registered active faults
        std::vector<SignalFault> faults;

        // Updates and generates random faults
        void updateFaults();

        // Checks if a signal already has an active fault
        bool hasActiveFault(SignalId id) const;

        // Gets the active fault type of a signal
        FaultType getFaultType(SignalId id) const;

        // Add a signal to the vector based on its failure state
        void addSignal(
            std::vector<SignalReading>& signals,
            SignalId id,
            double normalValue,
            double faultValue) const;

    public:

        VehicleSimulator();

        void updateSignal();

        std::vector<SignalReading> exposeSignals() const;
};
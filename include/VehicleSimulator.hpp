#pragma once

#include "SignalTypes.hpp"

#include <random>
#include <vector>

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

        std::vector<SignalFault> faults;

        void updateFaults();

        void activateFault(
            SignalId id,
            FaultType type,
            unsigned int duration
        );

        bool hasActiveFault(SignalId id) const;

        FaultType getFaultType(SignalId id) const;

        void addSignalReading(
            std::vector<SignalReading>& readings,
            SignalId id,
            double normalValue,
            double faultValue
        ) const;

    public:

        VehicleSimulator();

        void updateSignal();

        std::vector<SignalReading> exposeSignals() const;
};

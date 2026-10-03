#pragma once

#include <random>
#include <vector>
#include <algorithm>

struct SignalVehicle
{
    std::string name;
    double value;
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

    public:

        VehicleSimulator();

        void updateSignal();

        std::vector<SignalVehicle> exposeSignals() const;
        
};
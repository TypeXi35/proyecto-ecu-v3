
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Sensor.hpp"
#include "SignalTypes.hpp"

// Scaling parameters for each signal.
struct ScalingConfig
{
    double factor;
    double offset;
};

// Receives, processes and validates vehicle signals.
class GatewayECU
{
    private:
        // Sensors managed by the Gateway.
        std::vector<Sensor> sensors;

        // Gets the scaling parameters for a signal.
        static bool getScalingConfig(
            SignalId id,
            ScalingConfig &config);

        // Converts a physical value into a raw value.
        static bool encode(
            SignalId id,
            double physicalValue,
            std::uint16_t &rawValue);

        // Converts a raw value into a physical value.
        static bool decode(
            SignalId id,
            std::uint16_t rawValue,
            double &physicalValue);

    public:
        // Creates sensors with their valid operating ranges.
        GatewayECU();

        // Processes physical readings and updates sensor states.
        void processCycle(const std::vector<SignalReading> &readings);

        // Provides read-only access to the sensors.
        const std::vector<Sensor> &getSensors() const;

        // Finds a sensor by its ID.
        const Sensor &findSensor(SignalId id) const;

        // Counts sensors that are not in VALID state.
        std::size_t countInvalidSignals() const;
};

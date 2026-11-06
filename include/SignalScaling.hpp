
#pragma once

#include "SignalTypes.hpp"

#include <cstdint>
#include <vector>

// Scaling parameters associated with a signal.
struct ScalingConfig
{
    double factor;
    double offset;
};

class SignalScaling
{
    private:

        static bool getScalingConfig(
            SignalId id,
            ScalingConfig& config
        );

    public:
    
        static bool encode(
            SignalId id,
            double physicalValue,
            std::uint16_t& rawValue
        );

        static bool decode(
            SignalId id,
            std::uint16_t rawValue,
            double& physicalValue
        );

        static bool encodeSignals(
            const std::vector<SignalReading>& physicalSignals,
            std::vector<RawSignalReading>& rawSignals
        );
};

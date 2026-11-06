
#include "SignalScaling.hpp"

#include <cmath>
#include <limits>

bool SignalScaling::getScalingConfig(
    SignalId id,
    ScalingConfig& config)
{
    // Select the scaling parameters for each signal.
    switch (id)
    {
        case SignalId::SPEED:
            config = {0.1, 0.0};
            return true;

        case SignalId::RPM:
            config = {1.0, 0.0};
            return true;

        case SignalId::TEMPERATURE:
            config = {0.1, -40.0};
            return true;

        case SignalId::THROTTLE:
            config = {0.1, 0.0};
            return true;

        case SignalId::BATTERY_VOLTAGE:
            config = {0.01, 0.0};
            return true;

        case SignalId::OIL_PRESSURE:
            config = {0.05, 0.0};
            return true;
    }

    return false;
}

bool SignalScaling::encode(
    SignalId id,
    double physicalValue,
    std::uint16_t& rawValue)
{
    // Reject NaN and infinite values.
    if (!std::isfinite(physicalValue))
    {
        return false;
    }

    // Get the scaling configuration.
    ScalingConfig config;

    if (!getScalingConfig(id, config))
    {
        return false;
    }

    double calculatedRaw = (physicalValue - config.offset) / config.factor;

    if (!std::isfinite(calculatedRaw))
    {
        return false;
    }

    double roundedRaw = std::round(calculatedRaw);

    if (roundedRaw < 0.0 || roundedRaw > std::numeric_limits<std::uint16_t>::max())
    {
        return false;
    }

    rawValue = static_cast<std::uint16_t>(roundedRaw);

    return true;
}

bool SignalScaling::decode(
    SignalId id,
    std::uint16_t rawValue,
    double& physicalValue)
{
    // Get the scaling configuration.
    ScalingConfig config;

    if (!getScalingConfig(id, config))
    {
        return false;
    }

    double decodedValue = rawValue * config.factor + config.offset;

    if (!std::isfinite(decodedValue))
    {
        return false;
    }

    physicalValue = decodedValue;

    return true;
}


bool SignalScaling::encodeSignals(
    const std::vector<SignalReading>& physicalSignals,
    std::vector<RawSignalReading>& rawSignals)
{
    std::vector<RawSignalReading> encodedSignals;

    encodedSignals.reserve(physicalSignals.size());

    // Encode each physical signal.
    for (const SignalReading& signal : physicalSignals)
    {
        std::uint16_t rawValue = 0;

        bool success = encode(
            signal.id,
            signal.value,
            rawValue
        );

        if (!success)
        {
            return false;
        }

        encodedSignals.push_back({signal.id, rawValue});
    }

    rawSignals = std::move(encodedSignals);

    return true;
}

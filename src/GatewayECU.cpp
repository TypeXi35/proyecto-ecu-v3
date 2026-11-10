
#include "GatewayECU.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "SignalLimits.hpp"

GatewayECU::GatewayECU()
    : sensors{
          Sensor(SignalId::SPEED,           "km/h", SPEED_MIN_KMH,        SPEED_MAX_KMH,        MAX_MISSED_CYCLES),
          Sensor(SignalId::RPM,             "rpm",  RPM_MIN,              RPM_MAX,              MAX_MISSED_CYCLES),
          Sensor(SignalId::TEMPERATURE,     "C",    TEMP_MIN_C,           TEMP_MAX_C,           MAX_MISSED_CYCLES),
          Sensor(SignalId::THROTTLE,        "%",    THROTTLE_MIN_PCT,     THROTTLE_MAX_PCT,     MAX_MISSED_CYCLES),
          Sensor(SignalId::BATTERY_VOLTAGE, "V",    VOLTAGE_MIN_V,        VOLTAGE_MAX_V,        MAX_MISSED_CYCLES),
          Sensor(SignalId::OIL_PRESSURE,    "bar",  OIL_PRESSURE_MIN_BAR, OIL_PRESSURE_MAX_BAR, MAX_MISSED_CYCLES),
      }
{
}

bool GatewayECU::getScalingConfig(
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

        default:
            return false;
    }
}

bool GatewayECU::encode(
    SignalId id,
    double physicalValue,
    std::uint16_t& rawValue)
{
    if (!std::isfinite(physicalValue))
    {
        return false;
    }

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

bool GatewayECU::decode(
    SignalId id,
    std::uint16_t rawValue,
    double& physicalValue)
{
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

void GatewayECU::processCycle(
    const std::vector<SignalReading>& readings)
{
    for (Sensor& sensor : sensors)
    {
        auto reading = std::find_if(
            readings.begin(),
            readings.end(),
            [&sensor](const SignalReading& candidate)
            {
                return candidate.id == sensor.getId();
            }
        );

        if (reading == readings.end())
        {
            sensor.registerMissedCycle();
            continue;
        }

        std::uint16_t rawValue = 0;
        double physicalValue = 0.0;

        bool encoded = encode(
            reading->id,
            reading->value,
            rawValue
        );

        if (!encoded)
        {
            sensor.registerMissedCycle();
            continue;
        }

        bool decoded = decode(
            reading->id,
            rawValue,
            physicalValue
        );

        if (!decoded)
        {
            sensor.registerMissedCycle();
            continue;
        }

        sensor.update(physicalValue);
    }
}

const std::vector<Sensor>& GatewayECU::getSensors() const
{
    return sensors;
}

const Sensor& GatewayECU::findSensor(SignalId id) const
{
    const auto found = std::find_if(
        sensors.begin(),
        sensors.end(),
        [id](const Sensor& sensor)
        {
            return sensor.getId() == id;
        }
    );

    if (found == sensors.end())
    {
        throw std::out_of_range(
            "GatewayECU::findSensor: la Gateway no tiene un sensor con ese ID"
        );
    }

    return *found;
}

std::size_t GatewayECU::countInvalidSignals() const
{
    const auto invalid = std::count_if(
        sensors.begin(),
        sensors.end(),
        [](const Sensor& sensor)
        {
            return sensor.getState() != SignalState::VALID;
        }
    );

    return static_cast<std::size_t>(invalid);
}

#include "GatewayECU.hpp"

#include <algorithm>
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
      } {
}

void GatewayECU::processCycle(const std::vector<SignalReading>& readings) {
    std::for_each(sensors.begin(), sensors.end(), [&readings](Sensor& sensor) {
        const auto reading = std::find_if(
            readings.begin(), readings.end(),
            [&sensor](const SignalReading& candidate) { return candidate.id == sensor.getId(); });

        if (reading != readings.end()) {
            sensor.update(reading->value);
        } else {
            sensor.registerMissedCycle();
        }
    });
}

const std::vector<Sensor>& GatewayECU::getSensors() const {
    return sensors;
}

const Sensor& GatewayECU::findSensor(SignalId id) const {
    const auto found = std::find_if(sensors.begin(), sensors.end(),
                                    [id](const Sensor& sensor) { return sensor.getId() == id; });
    if (found == sensors.end()) {
        throw std::out_of_range("GatewayECU::findSensor: la Gateway no tiene un sensor con ese ID");
    }
    return *found;
}

std::size_t GatewayECU::countInvalidSignals() const {
    const auto invalid = std::count_if(sensors.begin(), sensors.end(), [](const Sensor& sensor) {
        return sensor.getState() != SignalState::VALID;
    });
   
    return static_cast<std::size_t>(invalid);
}

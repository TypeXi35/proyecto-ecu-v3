#include "Logger.hpp"

namespace {
    SensorInfo toInfo(const Sensor& sensor)
    {
        return SensorInfo{
            sensor.getValue(),
            sensor.getMin(),
            sensor.getMax(),
            sensor.getMissedCycles(),
            sensor.getMaxMissedCycles(),
            sensor.getState()
        };
    }
}

void Logger::updateData(const ECUData& ecu)
{
    current.cycleCount     = ecu.cycleCount;
    current.state          = ecu.currentState;
    current.speed          = toInfo(ecu.sensors.speed);
    current.rpm            = toInfo(ecu.sensors.rpm);
    current.temperature    = toInfo(ecu.sensors.temperature);
    current.batteryVoltage = toInfo(ecu.sensors.batteryVoltage);
    current.throttle       = toInfo(ecu.sensors.throttle);
    current.oilPressure    = toInfo(ecu.sensors.oilPressure);
}

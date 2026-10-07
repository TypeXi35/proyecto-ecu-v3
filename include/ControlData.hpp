#pragma once

#include "Sensor.hpp"
#include "ECUState.hpp"

struct ControlSensors {
    const Sensor& speed;
    const Sensor& rpm;
    const Sensor& temperature;
    const Sensor& batteryVoltage;
    const Sensor& throttle;
    const Sensor& oilPressure;
};

struct ECUData {
    ControlSensors sensors;
    const ECUState& currentState;
};
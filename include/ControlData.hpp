#pragma once

#include "Sensor.hpp"
#include "ECUState.hpp"

struct ControlSensors {
    Sensor speed;
    Sensor rpm;
    Sensor temperature;
    Sensor batteryVoltage;
    Sensor throttle;
    Sensor oilPressure;
};

enum class FaultCode{
    NONE,
    CRITICAL_SENSOR_LOST,
    CRITICAL_OUT_OF_LIMITS,
    SENSOR_DEGRADED,
    SIGNAL_OUT_OF_LIMITS,
    INCOHERENT_SIGNALS
};

struct ECUData {
    ControlSensors sensors;
    ECUState currentState;
    unsigned int cycleCount;
    FaultCode faultCode;
};
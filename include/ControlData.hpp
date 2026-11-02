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
    const ECUState& currentState;
    const unsigned int& cycleCount;
    const FaultCode &faultCode;
};
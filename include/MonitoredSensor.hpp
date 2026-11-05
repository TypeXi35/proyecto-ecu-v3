#pragma once

#include "Sensor.hpp"

enum class Criticality {
    CRITICAL = 1,
    NON_CRITICAL = 0
};

struct MonitoredSensor {
    const Sensor &sensor;
    double operatingMin;
    double operatingMax;
    Criticality criticality;
};

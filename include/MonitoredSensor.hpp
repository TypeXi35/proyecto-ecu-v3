#pragma once

#include "Sensor.hpp"

struct MonitoredSensor {
    const Sensor &sensor;
    double operatingMin;
    double operatingMax;
};

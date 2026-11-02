#pragma once

#include "Sensor.hpp"

struct MonitoredSignal{
    const Sensor *sensor;
    double floor;
    double ceiling;
};
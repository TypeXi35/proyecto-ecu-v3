#pragma once

// Límites físicos de cada sensor

constexpr double SPEED_MIN_KMH        =    0.0;
constexpr double SPEED_MAX_KMH        =  250.0;

constexpr double RPM_MIN              =    0.0;
constexpr double RPM_MAX              = 8000.0;

constexpr double TEMP_MIN_C           =  -40.0;
constexpr double TEMP_MAX_C           =  150.0;

constexpr double THROTTLE_MIN_PCT     =    0.0;
constexpr double THROTTLE_MAX_PCT     =  100.0;

constexpr double VOLTAGE_MIN_V        =    9.0;
constexpr double VOLTAGE_MAX_V        =   16.0;

constexpr double OIL_PRESSURE_MIN_BAR =    0.0;
constexpr double OIL_PRESSURE_MAX_BAR =   10.0;

// Ciclos sin dato tolerados antes de NOT_AVAILABLE
constexpr unsigned int MAX_MISSED_CYCLES = 3;

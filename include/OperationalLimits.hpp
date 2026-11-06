#pragma once

// Límites operativos que vigila ControlECU

// Misma relación que usó Alan en el simulador, rpm = 800 + 22 * velocidad + 18 * acelerador
constexpr double IDLE_RPM = 800.0;
constexpr double RPM_PER_KMH = 22.0;
constexpr double RPM_PER_THROTTLE = 18.0;
constexpr double SPEED_TOLERANCE = 15.0;
constexpr double HIGH_THROTTLE = 80.0;
constexpr double MIN_RPM_UNDER_LOAD = 500.0;

constexpr double SPEED_OPERATING_MIN_KMH = 0.0;
constexpr double SPEED_OPERATING_MAX_KMH = 200.0;
constexpr double RPM_OPERATING_MIN = 0.0; // no absolute minimum: low RPM is judged in context by isIncoherent()
constexpr double RPM_OPERATING_MAX = 6800.0;
constexpr double TEMPERATURE_OPERATING_MIN_C = -30.0;
constexpr double TEMPERATURE_OPERATING_MAX_C = 110.0;
constexpr double THROTTLE_OPERATING_MIN_PCT = 0.0;
constexpr double THROTTLE_OPERATING_MAX_PCT = 100.0;
constexpr double BATTERY_OPERATING_MIN_V = 11.5;
constexpr double BATTERY_OPERATING_MAX_V = 15.0;
constexpr double OIL_PRESSURE_OPERATING_MIN_BAR = 0.8;
constexpr double OIL_PRESSURE_OPERATING_MAX_BAR = 6.0;

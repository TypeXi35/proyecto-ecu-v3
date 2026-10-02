#pragma once

// Tipos compartidos entre Simulador, Gateway, Control y Dashboard
enum class SignalId {
    SPEED,
    RPM,
    TEMPERATURE,
    THROTTLE,
    BATTERY_VOLTAGE,
    OIL_PRESSURE
};

// Estado de validez que determina la Gateway
enum class SignalState {
    VALID,
    OUT_OF_RANGE,
    NOT_AVAILABLE   // Sin primer dato o dejó de actualizarse
};

// Lectura de una señal en un ciclo; si no viene en la lista, no llegó
struct SignalReading {
    SignalId id;
    double value;
};

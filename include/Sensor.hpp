#pragma once

#include <string>

#include "SignalTypes.hpp"

// Guarda el valor de una señal y se valida a sí mismo
class Sensor {
public:
    // Inicia en 0.0 y NOT_AVAILABLE; maxMissed = ciclos sin dato tolerados
    Sensor(SignalId signalId, std::string signalUnit, double minimum, double maximum,
           unsigned int maxMissed);

    // Guarda el valor, reinicia el contador y revalida
    void update(double newValue);

    // Suma un ciclo sin dato; al superar el límite pasa a NOT_AVAILABLE
    void registerMissedCycle();

    SignalId getId() const;
    const std::string& getUnit() const;
    double getValue() const;
    double getMin() const;
    double getMax() const;
    unsigned int getMissedCycles() const;
    SignalState getState() const;

private:
    // Determina el estado según el valor actual
    void validate();

    SignalId id;
    std::string unit;
    double value{0.0};
    double minValue;
    double maxValue;
    unsigned int missedCycles{0};
    unsigned int maxMissedCycles;
    SignalState state{SignalState::NOT_AVAILABLE};
};

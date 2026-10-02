#include "Sensor.hpp"

#include <utility>

Sensor::Sensor(SignalId signalId, std::string signalUnit, double minimum, double maximum,
               unsigned int maxMissed)
    : id(signalId),
      unit(std::move(signalUnit)),
      minValue(minimum),
      maxValue(maximum),
      maxMissedCycles(maxMissed) {
}

void Sensor::update(double newValue) {
    value = newValue;
    missedCycles = 0;
    validate();
}

void Sensor::registerMissedCycle() {
    ++missedCycles;
    if (missedCycles > maxMissedCycles) {
        state = SignalState::NOT_AVAILABLE;
    }
}

void Sensor::validate() {
    // Se pregunta si está dentro para que NaN quede OUT_OF_RANGE
    const bool inRange = value >= minValue && value <= maxValue;
    state = inRange ? SignalState::VALID : SignalState::OUT_OF_RANGE;
}

SignalId Sensor::getId() const {
    return id;
}

const std::string& Sensor::getUnit() const {
    return unit;
}

double Sensor::getValue() const {
    return value;
}

double Sensor::getMin() const {
    return minValue;
}

double Sensor::getMax() const {
    return maxValue;
}

unsigned int Sensor::getMissedCycles() const {
    return missedCycles;
}

SignalState Sensor::getState() const {
    return state;
}

#pragma once

// Sirve para la Gateway y el Dashboard.
// Cambiar la fuente de las lecturas una vez que el simulador mande std::vector<SignalReading> con fallas.

#include <optional>
#include <string>
#include <vector>

#include "SignalTypes.hpp"

// La señal no llegó en ese ciclo
inline constexpr std::nullopt_t NO_DATA = std::nullopt;

// Lo que manda el simulador en un ciclo. NO_DATA es que la señal no llegó
struct ScenarioCycle {
    std::string description;
    std::optional<double> speed;
    std::optional<double> rpm;
    std::optional<double> temperature;
    std::optional<double> throttle;
    std::optional<double> batteryVoltage;
    std::optional<double> oilPressure;
};

// Orden de las columnas
inline const std::vector<SignalId> SCENARIO_SIGNALS = {
    SignalId::SPEED,    SignalId::RPM,             SignalId::TEMPERATURE,
    SignalId::THROTTLE, SignalId::BATTERY_VOLTAGE, SignalId::OIL_PRESSURE,
};

// Valor que trae el ciclo para una señal
inline std::optional<double> valueOf(const ScenarioCycle& cycle, SignalId id) {
    switch (id) {
        case SignalId::SPEED:           return cycle.speed;
        case SignalId::RPM:             return cycle.rpm;
        case SignalId::TEMPERATURE:     return cycle.temperature;
        case SignalId::THROTTLE:        return cycle.throttle;
        case SignalId::BATTERY_VOLTAGE: return cycle.batteryVoltage;
        case SignalId::OIL_PRESSURE:    return cycle.oilPressure;
    }
    return NO_DATA;
}

// Convierte un ciclo en las lecturas que recibe la Gateway
inline std::vector<SignalReading> toReadings(const ScenarioCycle& cycle) {
    std::vector<SignalReading> readings;
    for (const SignalId id : SCENARIO_SIGNALS) {
        const std::optional<double> value = valueOf(cycle, id);
        if (value.has_value()) {
            readings.push_back({id, *value});
        }
    }
    return readings;
}

// Recorrido de 15 ciclos con faltas cortas y largas, una señal fuera de rango que deja de llegar y ausencia total
inline std::vector<ScenarioCycle> driveScenario() {
    return {
        //  descripción                                                      vel    rpm     temp   acel   bat      aceite
        {"arranque normal",                                                  60.0,  2200.0, 88.0,  30.0,  13.8,    2.1},
        {"valores normales",                                                 62.0,  2250.0, 88.5,  32.0,  13.8,    2.2},
        {"temperatura a 180 C",                                              63.0,  2270.0, 180.0, 32.0,  13.9,    2.2},
        {"temperatura regresa a 89 C; deja de llegar la bateria",            63.0,  2280.0, 89.0,  33.0,  NO_DATA, 2.2},
        {"sin bateria (2 ciclos)",                                           64.0,  2300.0, 89.2,  33.0,  NO_DATA, 2.3},
        {"sin bateria (3 ciclos); RPM a 9000",                               64.0,  9000.0, 89.4,  34.0,  NO_DATA, 2.3},
        {"sin bateria (4 ciclos); RPM normal",                               65.0,  2320.0, 89.5,  34.0,  NO_DATA, 2.3},
        {"sin bateria (5 ciclos); velocidad a -5 km/h; deja de llegar el aceite",
                                                                             -5.0,  2330.0, 89.6,  35.0,  NO_DATA, NO_DATA},
        {"regresa la bateria; velocidad normal; sin aceite (2 ciclos)",      66.0,  2340.0, 89.7,  35.0,  13.6,    NO_DATA},
        {"regresa el aceite; acelerador a 120 %",                            66.0,  2350.0, 89.8,  120.0, 13.7,    2.4},
        {"no llega ninguna senal (1 ciclo)",                                 NO_DATA, NO_DATA, NO_DATA, NO_DATA, NO_DATA, NO_DATA},
        {"no llega ninguna senal (2 ciclos)",                                NO_DATA, NO_DATA, NO_DATA, NO_DATA, NO_DATA, NO_DATA},
        {"no llega ninguna senal (3 ciclos)",                                NO_DATA, NO_DATA, NO_DATA, NO_DATA, NO_DATA, NO_DATA},
        {"no llega ninguna senal (4 ciclos)",                                NO_DATA, NO_DATA, NO_DATA, NO_DATA, NO_DATA, NO_DATA},
        {"regresan las 6 senales",                                           67.0,  2360.0, 90.0,  36.0,  13.8,    2.4},
    };
}

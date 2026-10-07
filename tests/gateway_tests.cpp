// Pruebas de la Gateway con objetos literales

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "DriveScenario.hpp"
#include "GatewayECU.hpp"
#include "SignalLimits.hpp"

namespace {

int failures = 0;

// Imprime el resultado de una comprobación y cuenta los fallos
void check(bool condition, const std::string& description) {
    std::cout << (condition ? "[OK]    " : "[FALLO] ") << description << '\n';
    if (!condition) {
        ++failures;
    }
}

// Texto del estado para los mensajes
std::string toText(SignalState state) {
    switch (state) {
        case SignalState::VALID:         return "VALID";
        case SignalState::OUT_OF_RANGE:  return "OUT_OF_RANGE";
        case SignalState::NOT_AVAILABLE: return "NOT_AVAILABLE";
    }
    return "DESCONOCIDO";
}

// Sensor de temperatura para las pruebas (-40 a 150 C)
Sensor makeTemperatureSensor() {
    return Sensor(SignalId::TEMPERATURE, "C", TEMP_MIN_C, TEMP_MAX_C, MAX_MISSED_CYCLES);
}

// Busca un sensor por ID; si no existe, termina las pruebas
const Sensor& findSensor(const GatewayECU& gateway, SignalId id) {
    const std::vector<Sensor>& sensors = gateway.getSensors();
    const auto found = std::find_if(sensors.begin(), sensors.end(),
                                    [id](const Sensor& sensor) { return sensor.getId() == id; });
    if (found == sensors.end()) {
        std::cout << "[FALLO] la Gateway no tiene el sensor buscado\n";
        std::exit(1);
    }
    return *found;
}

// Ciclo con las 6 señales en valores normales
std::vector<SignalReading> allValidReadings() {
    return {
        {SignalId::SPEED,           82.4},
        {SignalId::RPM,             2840.2},
        {SignalId::TEMPERATURE,     91.3},
        {SignalId::THROTTLE,        35.0},
        {SignalId::BATTERY_VOLTAGE, 13.8},
        {SignalId::OIL_PRESSURE,    2.7},
    };
}

const std::string LIMIT_TEXT = std::to_string(MAX_MISSED_CYCLES);
const std::string LIMIT_PLUS_ONE_TEXT = std::to_string(MAX_MISSED_CYCLES + 1);

// ---------------- Entregable 1: validación por rango ----------------

void testInitialState() {
    std::cout << "\n-- Sensor recien creado --\n";
    const Sensor sensor = makeTemperatureSensor();

    check(sensor.getState() == SignalState::NOT_AVAILABLE, "inicia NOT_AVAILABLE");
    check(sensor.getValue() == 0.0, "inicia con valor 0.0");
    check(sensor.getMissedCycles() == 0, "inicia con 0 ciclos sin dato");
}

void testValueInsideRange() {
    std::cout << "\n-- Valor dentro de rango --\n";
    Sensor sensor = makeTemperatureSensor();

    sensor.update(85.3);
    check(sensor.getState() == SignalState::VALID, "85.3 C es VALID");
    check(sensor.getValue() == 85.3, "guarda el valor 85.3");
}

void testRangeLimits() {
    std::cout << "\n-- Limites del rango (cuentan como validos) --\n";
    Sensor sensor = makeTemperatureSensor();

    sensor.update(TEMP_MIN_C);
    check(sensor.getState() == SignalState::VALID, "-40 C (minimo) es VALID");

    sensor.update(TEMP_MAX_C);
    check(sensor.getState() == SignalState::VALID, "150 C (maximo) es VALID");
}

void testValueOutOfRange() {
    std::cout << "\n-- Valor fuera de rango --\n";
    Sensor sensor = makeTemperatureSensor();

    sensor.update(180.0);
    check(sensor.getState() == SignalState::OUT_OF_RANGE, "180 C es OUT_OF_RANGE");
    check(sensor.getValue() == 180.0, "guarda el valor fuera de rango (180)");

    sensor.update(-40.1);
    check(sensor.getState() == SignalState::OUT_OF_RANGE, "-40.1 C (debajo del minimo) es OUT_OF_RANGE");

    sensor.update(150.1);
    check(sensor.getState() == SignalState::OUT_OF_RANGE, "150.1 C (arriba del maximo) es OUT_OF_RANGE");
}

void testNonNumericValues() {
    std::cout << "\n-- Valores no numericos --\n";
    Sensor sensor = makeTemperatureSensor();

    sensor.update(std::numeric_limits<double>::quiet_NaN());
    check(sensor.getState() == SignalState::OUT_OF_RANGE, "NaN es OUT_OF_RANGE");

    sensor.update(std::numeric_limits<double>::infinity());
    check(sensor.getState() == SignalState::OUT_OF_RANGE, "+infinito es OUT_OF_RANGE");

    sensor.update(-std::numeric_limits<double>::infinity());
    check(sensor.getState() == SignalState::OUT_OF_RANGE, "-infinito es OUT_OF_RANGE");
}

void testImmediateRecovery() {
    std::cout << "\n-- Recuperacion inmediata --\n";
    Sensor sensor = makeTemperatureSensor();

    sensor.update(180.0);
    sensor.update(90.0);
    check(sensor.getState() == SignalState::VALID, "180 C y luego 90 C vuelve a VALID");
}

void testGatewaySensors() {
    std::cout << "\n-- Sensores de la Gateway --\n";
    const GatewayECU gateway;
    const std::vector<Sensor>& sensors = gateway.getSensors();

    check(sensors.size() == 6, "crea 6 sensores");

    const bool allNotAvailable = std::all_of(
        sensors.begin(), sensors.end(),
        [](const Sensor& sensor) { return sensor.getState() == SignalState::NOT_AVAILABLE; });
    check(allNotAvailable, "todos inician NOT_AVAILABLE");

    struct ExpectedSensor {
        SignalId id{};
        std::string unit;
        double min{0.0};
        double max{0.0};
        std::string label;
    };

    const std::vector<ExpectedSensor> expectedSensors = {
        {SignalId::SPEED,           "km/h", SPEED_MIN_KMH,        SPEED_MAX_KMH,        "Velocidad"},
        {SignalId::RPM,             "rpm",  RPM_MIN,              RPM_MAX,              "RPM"},
        {SignalId::TEMPERATURE,     "C",    TEMP_MIN_C,           TEMP_MAX_C,           "Temperatura"},
        {SignalId::THROTTLE,        "%",    THROTTLE_MIN_PCT,     THROTTLE_MAX_PCT,     "Acelerador"},
        {SignalId::BATTERY_VOLTAGE, "V",    VOLTAGE_MIN_V,        VOLTAGE_MAX_V,        "Voltaje bateria"},
        {SignalId::OIL_PRESSURE,    "bar",  OIL_PRESSURE_MIN_BAR, OIL_PRESSURE_MAX_BAR, "Presion aceite"},
    };

    for (const ExpectedSensor& expected : expectedSensors) {
        const auto hasExpectedId = [&expected](const Sensor& sensor) {
            return sensor.getId() == expected.id;
        };

        const auto count = std::count_if(sensors.begin(), sensors.end(), hasExpectedId);
        check(count == 1, expected.label + ": existe una sola vez");

        const auto found = std::find_if(sensors.begin(), sensors.end(), hasExpectedId);
        const bool rangeMatches = found != sensors.end()
                                  && found->getMin() == expected.min
                                  && found->getMax() == expected.max;
        check(rangeMatches, expected.label + ": tiene el rango esperado");

        const bool unitMatches = found != sensors.end() && found->getUnit() == expected.unit;
        check(unitMatches, expected.label + ": tiene la unidad esperada");
    }
}

// ---------------- Entregable 2: supervisión de ciclos sin dato ----------------

void testMissedCyclesLimit() {
    std::cout << "\n-- Limite de ciclos sin dato --\n";
    Sensor sensor = makeTemperatureSensor();
    sensor.update(85.0);

    for (unsigned int cycle = 0; cycle < MAX_MISSED_CYCLES; ++cycle) {
        sensor.registerMissedCycle();
    }
    check(sensor.getMissedCycles() == MAX_MISSED_CYCLES, "cuenta " + LIMIT_TEXT + " ciclos sin dato");
    check(sensor.getState() == SignalState::VALID,
          "con " + LIMIT_TEXT + " ciclos sin dato sigue VALID (se toleran)");
    check(sensor.getValue() == 85.0, "conserva el ultimo valor recibido");

    sensor.registerMissedCycle();
    check(sensor.getState() == SignalState::NOT_AVAILABLE,
          "con " + LIMIT_PLUS_ONE_TEXT + " ciclos sin dato pasa a NOT_AVAILABLE");
}

void testCounterResetsOnArrival() {
    std::cout << "\n-- El contador se reinicia al llegar un dato --\n";
    Sensor sensor = makeTemperatureSensor();
    sensor.update(85.0);

    sensor.registerMissedCycle();
    sensor.registerMissedCycle();
    check(sensor.getMissedCycles() == 2, "cuenta 2 ciclos sin dato");

    sensor.update(86.0);
    check(sensor.getMissedCycles() == 0, "al llegar 86 C el contador vuelve a 0");
}

void testSlide14Table() {
    std::cout << "\n-- Tabla de la diapositiva 14 (con limite de " + LIMIT_TEXT + " ciclos) --\n";
    // Tabla del PDF: dos datos y luego ciclos sin dato según MAX_MISSED_CYCLES
    GatewayECU gateway;
    int cycle = 100;

    const auto checkCycle = [&gateway, &cycle](const std::vector<SignalReading>& readings,
                                               SignalState expected) {
        gateway.processCycle(readings);
        const std::string arrived = readings.empty() ? "sin dato" : "llega dato";
        check(findSensor(gateway, SignalId::TEMPERATURE).getState() == expected,
              "ciclo " + std::to_string(cycle) + " (" + arrived + "): " + toText(expected));
        ++cycle;
    };

    checkCycle({{SignalId::TEMPERATURE, 85.1}}, SignalState::VALID);
    checkCycle({{SignalId::TEMPERATURE, 85.3}}, SignalState::VALID);
    for (unsigned int missed = 0; missed < MAX_MISSED_CYCLES; ++missed) {
        checkCycle({}, SignalState::VALID);
    }
    checkCycle({}, SignalState::NOT_AVAILABLE);
}

void testOutOfRangeThenSilence() {
    std::cout << "\n-- Fuera de rango y luego deja de llegar --\n";
    GatewayECU gateway;

    gateway.processCycle({{SignalId::TEMPERATURE, 180.0}});
    check(findSensor(gateway, SignalId::TEMPERATURE).getState() == SignalState::OUT_OF_RANGE,
          "llega 180 C: OUT_OF_RANGE");

    for (unsigned int cycle = 0; cycle < MAX_MISSED_CYCLES; ++cycle) {
        gateway.processCycle({});
    }
    check(findSensor(gateway, SignalId::TEMPERATURE).getState() == SignalState::OUT_OF_RANGE,
          LIMIT_TEXT + " ciclos sin dato: sigue OUT_OF_RANGE");

    gateway.processCycle({});
    check(findSensor(gateway, SignalId::TEMPERATURE).getState() == SignalState::NOT_AVAILABLE,
          LIMIT_PLUS_ONE_TEXT + " ciclos sin dato: NOT_AVAILABLE");
}

void testRecoveryAfterNotAvailable() {
    std::cout << "\n-- Recuperacion despues de NOT_AVAILABLE --\n";
    GatewayECU gateway;

    gateway.processCycle({{SignalId::TEMPERATURE, 90.0}});
    for (unsigned int cycle = 0; cycle <= MAX_MISSED_CYCLES; ++cycle) {
        gateway.processCycle({});
    }
    check(findSensor(gateway, SignalId::TEMPERATURE).getState() == SignalState::NOT_AVAILABLE,
          "tras " + LIMIT_PLUS_ONE_TEXT + " ciclos sin dato: NOT_AVAILABLE");

    gateway.processCycle({{SignalId::TEMPERATURE, 91.0}});
    const Sensor& temperature = findSensor(gateway, SignalId::TEMPERATURE);
    check(temperature.getState() == SignalState::VALID, "llega 91 C: vuelve a VALID de inmediato");
    check(temperature.getMissedCycles() == 0, "y su contador vuelve a 0");
}

void testSignalThatNeverArrives() {
    std::cout << "\n-- Senal que nunca llega --\n";
    GatewayECU gateway;

    bool alwaysNotAvailable = true;
    for (unsigned int cycle = 0; cycle <= MAX_MISSED_CYCLES; ++cycle) {
        gateway.processCycle({{SignalId::SPEED, 50.0}});
        if (findSensor(gateway, SignalId::OIL_PRESSURE).getState() != SignalState::NOT_AVAILABLE) {
            alwaysNotAvailable = false;
        }
    }
    check(alwaysNotAvailable, "la presion de aceite nunca llega: NOT_AVAILABLE en todos los ciclos");
    check(findSensor(gateway, SignalId::SPEED).getState() == SignalState::VALID,
          "la velocidad, que si llega, esta VALID");
}

void testSignalsAreIndependent() {
    std::cout << "\n-- Cada senal se supervisa por separado --\n";
    GatewayECU gateway;
    const std::vector<Sensor>& sensors = gateway.getSensors();
    const auto isValid = [](const Sensor& sensor) { return sensor.getState() == SignalState::VALID; };

    gateway.processCycle(allValidReadings());
    check(std::all_of(sensors.begin(), sensors.end(), isValid), "llegan las 6 senales: todas VALID");

    std::vector<SignalReading> withoutTemperature = allValidReadings();
    withoutTemperature.erase(
        std::remove_if(withoutTemperature.begin(), withoutTemperature.end(),
                       [](const SignalReading& reading) { return reading.id == SignalId::TEMPERATURE; }),
        withoutTemperature.end());

    for (unsigned int cycle = 0; cycle <= MAX_MISSED_CYCLES; ++cycle) {
        gateway.processCycle(withoutTemperature);
    }
    check(findSensor(gateway, SignalId::TEMPERATURE).getState() == SignalState::NOT_AVAILABLE,
          "deja de llegar la temperatura: NOT_AVAILABLE");
    check(std::count_if(sensors.begin(), sensors.end(), isValid) == 5, "las otras 5 siguen VALID");
}

// ---------------- Consultas para la ECU de Control y el Dashboard ----------------

void testFindSensorReturnsGatewaySensor() {
    std::cout << "\n-- findSensor devuelve el sensor de la Gateway --\n";
    GatewayECU gateway;

    const std::vector<SignalId> ids = {
        SignalId::SPEED,    SignalId::RPM,             SignalId::TEMPERATURE,
        SignalId::THROTTLE, SignalId::BATTERY_VOLTAGE, SignalId::OIL_PRESSURE,
    };
    // Misma dirección = es el sensor que guarda la Gateway, no una copia
    const bool allFound = std::all_of(ids.begin(), ids.end(), [&gateway](SignalId id) {
        const Sensor& found = gateway.findSensor(id);
        return found.getId() == id && &found == &findSensor(gateway, id);
    });
    check(allFound, "encuentra las 6 senales y devuelve el mismo objeto que guarda la Gateway");

    // La referencia sigue al sensor, refleja los ciclos posteriores
    const Sensor& temperature = gateway.findSensor(SignalId::TEMPERATURE);
    gateway.processCycle({{SignalId::TEMPERATURE, 91.3}});
    check(temperature.getValue() == 91.3 && temperature.getState() == SignalState::VALID,
          "la referencia obtenida antes del ciclo ya muestra 91.3 C VALID");
}

void testFindSensorUnknownId() {
    std::cout << "\n-- findSensor con un ID que no existe --\n";
    const GatewayECU gateway;

    // Un enum class acepta cualquier valor de su tipo base, 99 no es una señal de la Gateway
    const auto unknownId = static_cast<SignalId>(99);
    bool threw = false;
    try {
        static_cast<void>(gateway.findSensor(unknownId));
    } catch (const std::out_of_range&) {
        threw = true;
    }
    check(threw, "un ID que no existe lanza std::out_of_range");
}

void testCountInvalidSignals() {
    std::cout << "\n-- countInvalidSignals --\n";
    GatewayECU gateway;
    check(gateway.countInvalidSignals() == 6, "recien creada: 6 invalidas (todas NOT_AVAILABLE)");

    gateway.processCycle(allValidReadings());
    check(gateway.countInvalidSignals() == 0, "llegan las 6 en rango: 0 invalidas");

    // Temperatura fuera de rango y sin voltaje de batería
    const std::vector<SignalReading> faultyReadings = {
        {SignalId::SPEED,        82.4},
        {SignalId::RPM,          2840.2},
        {SignalId::TEMPERATURE,  180.0},
        {SignalId::THROTTLE,     35.0},
        {SignalId::OIL_PRESSURE, 2.7},
    };

    for (unsigned int cycle = 0; cycle < MAX_MISSED_CYCLES; ++cycle) {
        gateway.processCycle(faultyReadings);
    }
    check(gateway.countInvalidSignals() == 1,
          "temperatura a 180 C y bateria con " + LIMIT_TEXT + " ciclos sin dato (tolerados): 1 invalida");

    gateway.processCycle(faultyReadings);
    check(gateway.countInvalidSignals() == 2,
          "bateria con " + LIMIT_PLUS_ONE_TEXT
              + " ciclos sin dato: 2 invalidas (cuenta OUT_OF_RANGE y NOT_AVAILABLE)");

    gateway.processCycle(allValidReadings());
    check(gateway.countInvalidSignals() == 0, "vuelven las 6 en rango: 0 invalidas (recuperacion inmediata)");
}

void testCycleCount() {
    std::cout << "\n-- getCycleCount --\n";
    GatewayECU gateway;
    check(gateway.getCycleCount() == 0, "recien creada: ciclo 0");

    gateway.processCycle(allValidReadings());
    check(gateway.getCycleCount() == 1, "tras un ciclo con datos: ciclo 1");

    gateway.processCycle({});
    check(gateway.getCycleCount() == 2, "un ciclo sin ningun dato tambien cuenta: ciclo 2");
}

// ---------------- Recorrido de varios ciclos ----------------

// Los estados esperados del recorrido cuentan los ciclos sin dato con este límite
static_assert(MAX_MISSED_CYCLES == 3, "los estados esperados del recorrido suponen MAX_MISSED_CYCLES == 3");

// Nombre de la señal para los mensajes
std::string signalText(SignalId id) {
    switch (id) {
        case SignalId::SPEED:           return "velocidad";
        case SignalId::RPM:             return "rpm";
        case SignalId::TEMPERATURE:     return "temperatura";
        case SignalId::THROTTLE:        return "acelerador";
        case SignalId::BATTERY_VOLTAGE: return "bateria";
        case SignalId::OIL_PRESSURE:    return "aceite";
    }
    return "desconocida";
}

void testDriveScenario() {
    std::cout << "\n-- Recorrido con el guion de tests/DriveScenario.hpp --\n";

    // Abreviaturas de la tabla
    constexpr SignalState V = SignalState::VALID;
    constexpr SignalState R = SignalState::OUT_OF_RANGE;
    constexpr SignalState N = SignalState::NOT_AVAILABLE;

    // Estados esperados por ciclo, en el orden de las columnas
    const std::vector<std::vector<SignalState>> expectedStates = {
        // vel rpm temp acel bat aceite
        {V, V, V, V, V, V},  // 1
        {V, V, V, V, V, V},  // 2
        {V, V, R, V, V, V},  // 3
        {V, V, V, V, V, V},  // 4
        {V, V, V, V, V, V},  // 5
        {V, R, V, V, V, V},  // 6
        {V, V, V, V, N, V},  // 7
        {R, V, V, V, N, V},  // 8
        {V, V, V, V, V, V},  // 9
        {V, V, V, R, V, V},  // 10
        {V, V, V, R, V, V},  // 11
        {V, V, V, R, V, V},  // 12
        {V, V, V, R, V, V},  // 13
        {N, N, N, N, N, N},  // 14
        {V, V, V, V, V, V},  // 15
    };

    const std::vector<ScenarioCycle> scenario = driveScenario();
    const bool rowsComplete = std::all_of(
        expectedStates.begin(), expectedStates.end(),
        [](const std::vector<SignalState>& row) { return row.size() == SCENARIO_SIGNALS.size(); });
    check(scenario.size() == expectedStates.size() && rowsComplete,
          "la tabla de estados esperados cubre cada ciclo y cada senal del guion");
    if (scenario.size() != expectedStates.size() || !rowsComplete) {
        return;
    }

    GatewayECU gateway;
    // Último valor recibido de cada señal, la Gateway debe conservarlo mientras no llegue otro
    std::vector<double> lastValues(SCENARIO_SIGNALS.size(), 0.0);

    for (std::size_t index = 0; index < scenario.size(); ++index) {
        const ScenarioCycle& cycle = scenario[index];
        const std::vector<SignalState>& expected = expectedStates[index];
        gateway.processCycle(toReadings(cycle));

        std::vector<std::string> problems;
        for (std::size_t column = 0; column < SCENARIO_SIGNALS.size(); ++column) {
            const SignalId id = SCENARIO_SIGNALS[column];
            const Sensor& sensor = gateway.findSensor(id);

            const std::optional<double> arrived = valueOf(cycle, id);
            if (arrived.has_value()) {
                lastValues[column] = *arrived;
            }

            if (sensor.getState() != expected[column]) {
                problems.push_back(signalText(id) + ": " + toText(sensor.getState()) + ", se esperaba "
                                   + toText(expected[column]));
            }
            if (sensor.getValue() != lastValues[column]) {
                problems.push_back(signalText(id) + ": valor " + std::to_string(sensor.getValue())
                                   + ", se esperaba " + std::to_string(lastValues[column]));
            }
        }

        const auto expectedInvalid = std::count_if(expected.begin(), expected.end(), [](SignalState state) {
            return state != SignalState::VALID;
        });
        if (gateway.countInvalidSignals() != static_cast<std::size_t>(expectedInvalid)) {
            problems.push_back("countInvalidSignals: " + std::to_string(gateway.countInvalidSignals())
                               + ", se esperaba " + std::to_string(expectedInvalid));
        }

        check(problems.empty(), "ciclo " + std::to_string(index + 1) + ", " + cycle.description + " -> "
                                    + std::to_string(expectedInvalid) + " invalida(s)");
        for (const std::string& problem : problems) {
            std::cout << "          " << problem << '\n';
        }
    }
}

}  // namespace

int main() {
    std::cout << "===== Entregable 1: validacion por rango =====\n";
    testInitialState();
    testValueInsideRange();
    testRangeLimits();
    testValueOutOfRange();
    testNonNumericValues();
    testImmediateRecovery();
    testGatewaySensors();

    std::cout << "\n===== Entregable 2: supervision de ciclos sin dato =====\n";
    testMissedCyclesLimit();
    testCounterResetsOnArrival();
    testSlide14Table();
    testOutOfRangeThenSilence();
    testRecoveryAfterNotAvailable();
    testSignalThatNeverArrives();
    testSignalsAreIndependent();

    std::cout << "\n===== Parte 2: consultas para Control y Dashboard =====\n";
    testFindSensorReturnsGatewaySensor();
    testFindSensorUnknownId();
    testCountInvalidSignals();
    testCycleCount();

    std::cout << "\n===== Recorrido de varios ciclos =====\n";
    testDriveScenario();

    std::cout << '\n';
    if (failures == 0) {
        std::cout << "Todas las pruebas pasaron.\n";
        return 0;
    }
    std::cout << failures << " prueba(s) fallaron.\n";
    return 1;
}

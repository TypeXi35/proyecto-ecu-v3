// Pruebas del Dashboard con la Gateway, el guion de ciclos y estados de la ECU de Control

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "BrailleCanvas.hpp"
#include "ControlData.hpp"
#include "Dashboard.hpp"
#include "DriveScenario.hpp"
#include "GatewayECU.hpp"

namespace {

int failures = 0;

// Imprime el resultado de una comprobación y cuenta los fallos
void check(bool condition, const std::string& description) {
    std::cout << (condition ? "[OK]    " : "[FALLO] ") << description << '\n';
    if (!condition) {
        ++failures;
    }
}

// Gateway después de procesar los primeros ciclos del guion
GatewayECU gatewayAfter(std::size_t cycles) {
    GatewayECU gateway;
    const std::vector<ScenarioCycle> scenario = driveScenario();
    for (std::size_t index = 0; index < cycles && index < scenario.size(); ++index) {
        gateway.processCycle(toReadings(scenario[index]));
    }
    return gateway;
}

// Dibuja un cuadro en memoria en lugar de la pantalla
std::string render(const GatewayECU& gateway, ECUState controlState) {
    const unsigned int cycle = gateway.getCycleCount();
    const ECUData controlData{{gateway.findSensor(SignalId::SPEED), gateway.findSensor(SignalId::RPM),
                               gateway.findSensor(SignalId::TEMPERATURE),
                               gateway.findSensor(SignalId::BATTERY_VOLTAGE),
                               gateway.findSensor(SignalId::THROTTLE), gateway.findSensor(SignalId::OIL_PRESSURE)},
                              controlState,
                              cycle};
    std::ostringstream out;
    Dashboard(controlData, out).render();
    return out.str();
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }
    return lines;
}

// Quita las secuencias ANSI (ESC [ ... letra) para comparar solo el texto visible
std::string stripAnsi(const std::string& text) {
    std::string visible;
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '\033' && index + 1 < text.size() && text[index + 1] == '[') {
            index += 2;
            while (index < text.size() && std::isalpha(static_cast<unsigned char>(text[index])) == 0) {
                ++index;
            }
            continue;
        }
        visible += text[index];
    }
    return visible;
}

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

// Primera línea del cuadro que empieza con el texto dado; vacía si no hay
std::string lineStartingWith(const std::string& frame, const std::string& prefix) {
    const std::vector<std::string> lines = splitLines(frame);
    const auto found = std::find_if(lines.begin(), lines.end(), [&prefix](const std::string& line) {
        return line.rfind(prefix, 0) == 0;
    });
    return found != lines.end() ? *found : "";
}

// Muestra el cuadro obtenido cuando no coincide con el esperado
void checkFrame(const std::string& actual, const std::string& expected, const std::string& description) {
    check(actual == expected, description);
    if (actual != expected) {
        std::cout << "---- obtenido ----\n" << actual << "---- esperado ----\n" << expected;
    }
}

// ---------------- Tablero ----------------

// Colores del tablero (parámetros SGR)
const std::string GREEN = "38;5;42";
const std::string AMBER = "38;5;214";
const std::string RED   = "1;38;5;196";
const std::string DIM   = "38;5;238";

// Ancho en columnas: los caracteres de cuadro, bloque y braille ocupan 3 bytes y una columna
std::size_t displayWidth(const std::string& text) {
    return static_cast<std::size_t>(std::count_if(text.begin(), text.end(), [](char byte) {
        return (static_cast<unsigned char>(byte) & 0xC0) != 0x80;
    }));
}

// Indica si algún carácter braille está pintado con ese color; así se sabe si hay una aguja de ese color
bool hasBrailleIn(const std::string& frame, const std::string& color) {
    const std::string start = "\033[" + color + "m";
    for (std::size_t at = frame.find(start); at != std::string::npos; at = frame.find(start, at + 1)) {
        const std::size_t glyph = at + start.size();
        if (glyph + 1 < frame.size() && static_cast<unsigned char>(frame[glyph]) == 0xE2
            && static_cast<unsigned char>(frame[glyph + 1]) >= 0xA0
            && static_cast<unsigned char>(frame[glyph + 1]) <= 0xA3) {
            return true;
        }
    }
    return false;
}

void testBrailleCanvas() {
    std::cout << "\n-- BrailleCanvas --\n";
    check(BrailleCanvas::glyph(0x00) == "\xE2\xA0\x80" && BrailleCanvas::glyph(0xFF) == "\xE2\xA3\xBF",
          "el patron vacio es U+2800 y el de 8 puntos es U+28FF");

    BrailleCanvas canvas(2, 1);
    check(canvas.dots(0, 0) == 0 && canvas.dots(1, 0) == 0, "un lienzo nuevo no tiene puntos");

    canvas.setDot(0, 0);
    canvas.setDot(1, 3);
    check(canvas.dots(0, 0) == 0x81 && BrailleCanvas::glyph(canvas.dots(0, 0)) == "\xE2\xA2\x81",
          "los puntos de las esquinas opuestas de una celda forman U+2881");

    canvas.setDot(2.4, 0.6);
    check(canvas.dots(1, 0) == 0x02, "un punto con decimales va al punto mas cercano");

    canvas.setDot(-1, 0);
    canvas.setDot(4, 0);
    canvas.setDot(0, 4);
    check(canvas.dots(0, 0) == 0x81 && canvas.dots(1, 0) == 0x02 && canvas.dots(5, 5) == 0,
          "los puntos fuera del lienzo se ignoran");

    BrailleCanvas line(2, 1);
    line.drawLine(0, 0, 3, 0);
    check(line.dots(0, 0) == 0x09 && line.dots(1, 0) == 0x09, "una linea horizontal enciende la fila de arriba de las dos celdas");
}

void testClusterStartup() {
    std::cout << "\n-- Tablero del arranque (ciclo 0) --\n";
    const std::string expected =
R"frame(╭─ ECU GATEWAY / CONTROL ──────────────────────────────────────────── CICLO 0 ─╮
│                                                                              │
│           ⣀⣤⠤⠒⠒⠒⠒⠢⢤⣄⡀                                   ⣀⡤⠤⠒⠒⡖⠒⠢⠤⣄⡀          │
│        ⢀⡴⠚⠁⠘      ⠘ ⠙⠲⣄                              ⢀⡴⠚⠙⠄   ⠁   ⠜⠙⠲⣄        │
│       ⣰⠋   100  150   ⠈⢳⡀                           ⣰⢏    3  4 5    ⢈⢷⡀      │
│      ⣰⠧⠄               ⠤⢷⡀       ▲ ATENCION        ⣰⠃ ⠁ 2        6  ⠁ ⢳⡀     │
│      ⡇  50         200   ⡇                         ⣇⣀                ⢀⣀⡇     │
│     ⢸⠁                   ⢹         6 de 6         ⢸⠁   1          7    ⢹     │
│     ⠈⡇   0         250   ⡏       invalidas        ⠈⡇   0          8    ⡏     │
│      ⠙⠉       ---      ⠈⠙⠁                         ⠙⠉       ---      ⠈⠙⠁     │
│              km/h                                           rpm              │
│           VELOCIDAD                                     RPM x1000            │
│         NO DISPONIBLE                                 NO DISPONIBLE          │
│                                                                              │
│ TEMPERATURA   ░░░░░░░░░░░░░░░░░░░░░░     --- C   NO DISPONIBLE               │
│ ACELERADOR    ░░░░░░░░░░░░░░░░░░░░░░     --- %   NO DISPONIBLE               │
│ BATERIA       ░░░░░░░░░░░░░░░░░░░░░░     --- V   NO DISPONIBLE               │
│ ACEITE        ░░░░░░░░░░░░░░░░░░░░░░     --- bar NO DISPONIBLE               │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                      ECU DE CONTROL   INIT   │
╰──────────────────────────────────────────────────────────────────────────────╯
)frame";
    checkFrame(stripAnsi(render(gatewayAfter(0), ECUState::INIT)), expected,
               "relojes sin aguja, barras apagadas, todas NO DISPONIBLE y la ECU de Control en INIT");
}

void testClusterWithFaults() {
    std::cout << "\n-- Tablero del ciclo 8 del guion --\n";
    const std::string expected =
R"frame(╭─ ECU GATEWAY / CONTROL ──────────────────────────────────────────── CICLO 8 ─╮
│                                                                              │
│           ⣀⣤⠤⠒⠒⠒⠒⠢⢤⣄⡀                                   ⣀⡤⠤⠒⠒⡖⠒⠢⠤⣄⡀          │
│        ⢀⡴⠚⠁⠘      ⠘ ⠙⠲⣄                              ⢀⡴⠚⠙⠄   ⠁   ⠜⠙⠲⣄        │
│       ⣰⠋   100  150   ⠈⢳⡀                           ⣰⢏  ⡀ 3  4 5    ⢈⢷⡀      │
│      ⣰⠧⠄               ⠤⢷⡀       ▲ ATENCION        ⣰⠃ ⠁ 2⢦⡀      6  ⠁ ⢳⡀     │
│      ⡇  50         200   ⡇                         ⣇⣀     ⠙⢦⡀        ⢀⣀⡇     │
│     ⢸⠁      ⢀⣀⠤⠄         ⢹         2 de 6         ⢸⠁   1    ⠙⠆    7    ⢹     │
│     ⠈⡇  ⡠0⠒⠋⠉      250   ⡏       invalidas        ⠈⡇   0          8    ⡏     │
│      ⠙⠉       -5       ⠈⠙⠁                         ⠙⠉      2330      ⠈⠙⠁     │
│              km/h                                           rpm              │
│           VELOCIDAD                                     RPM x1000            │
│         FUERA DE RANGO                                    VALIDA             │
│                                                                              │
│ TEMPERATURA   ███████████████░░░░░░░    89.6 C   VALIDA                      │
│ ACELERADOR    ███████▋░░░░░░░░░░░░░░    35.0 %   VALIDA                      │
│ BATERIA       ░░░░░░░░░░░░░░░░░░░░░░     --- V   NO DISPONIBLE · 5 ciclos    │
│ ACEITE        █████░░░░░░░░░░░░░░░░░     2.3 bar VALIDA · 1 ciclo            │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                  ECU DE CONTROL   DEGRADED   │
╰──────────────────────────────────────────────────────────────────────────────╯
)frame";
    checkFrame(stripAnsi(render(gatewayAfter(8), ECUState::DEGRADED)), expected,
               "aguja de velocidad en el tope por -5 km/h, bateria sin dato y aceite con 1 ciclo sin dato");
}

void testClusterBlackout() {
    std::cout << "\n-- Tablero del ciclo 14 del guion (4 ciclos sin ninguna senal) --\n";
    const std::string expected =
R"frame(╭─ ECU GATEWAY / CONTROL ─────────────────────────────────────────── CICLO 14 ─╮
│                                                                              │
│           ⣀⣤⠤⠒⠒⠒⠒⠢⢤⣄⡀                                   ⣀⡤⠤⠒⠒⡖⠒⠢⠤⣄⡀          │
│        ⢀⡴⠚⠁⠘      ⠘ ⠙⠲⣄                              ⢀⡴⠚⠙⠄   ⠁   ⠜⠙⠲⣄        │
│       ⣰⠋   100  150   ⠈⢳⡀                           ⣰⢏    3  4 5    ⢈⢷⡀      │
│      ⣰⠧⠄               ⠤⢷⡀       ▲ ATENCION        ⣰⠃ ⠁ 2        6  ⠁ ⢳⡀     │
│      ⡇  50         200   ⡇                         ⣇⣀                ⢀⣀⡇     │
│     ⢸⠁                   ⢹         6 de 6         ⢸⠁   1          7    ⢹     │
│     ⠈⡇   0         250   ⡏       invalidas        ⠈⡇   0          8    ⡏     │
│      ⠙⠉       ---      ⠈⠙⠁                         ⠙⠉       ---      ⠈⠙⠁     │
│              km/h                                           rpm              │
│           VELOCIDAD                                     RPM x1000            │
│         NO DISPONIBLE                                 NO DISPONIBLE          │
│                                                                              │
│ TEMPERATURA   ░░░░░░░░░░░░░░░░░░░░░░     --- C   NO DISPONIBLE · 4 ciclos    │
│ ACELERADOR    ░░░░░░░░░░░░░░░░░░░░░░     --- %   NO DISPONIBLE · 4 ciclos    │
│ BATERIA       ░░░░░░░░░░░░░░░░░░░░░░     --- V   NO DISPONIBLE · 4 ciclos    │
│ ACEITE        ░░░░░░░░░░░░░░░░░░░░░░     --- bar NO DISPONIBLE · 4 ciclos    │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                ECU DE CONTROL   SAFE_STATE   │
╰──────────────────────────────────────────────────────────────────────────────╯
)frame";
    checkFrame(stripAnsi(render(gatewayAfter(14), ECUState::SAFE_STATE)), expected,
               "las 6 NO DISPONIBLE con sus ciclos sin dato y la ECU de Control en SAFE_STATE");
}

void testClusterLayout() {
    std::cout << "\n-- Un tablero por cada ciclo del guion --\n";
    const std::vector<ScenarioCycle> scenario = driveScenario();
    GatewayECU gateway;

    for (std::size_t cycle = 0; cycle <= scenario.size(); ++cycle) {
        if (cycle > 0) {
            gateway.processCycle(toReadings(scenario[cycle - 1]));
        }
        const std::string frame = render(gateway, ECUState::OPERATIONAL);
        const std::vector<std::string> lines = splitLines(stripAnsi(frame));
        const bool sameSize = lines.size() == 21 && std::all_of(lines.begin(), lines.end(), [](const std::string& line) {
            return displayWidth(line) == 80;
        });
        // Se reescribe encima del cuadro anterior: inicio, fin de cada línea y lo que quede abajo
        const bool redraws = frame.rfind("\033[H", 0) == 0 && frame.size() >= 3
                             && frame.compare(frame.size() - 3, 3, "\033[J") == 0;
        const bool clearsEachLine = splitLines(frame).size() == 22
                                    && std::count(frame.begin(), frame.end(), '\n')
                                           == static_cast<std::ptrdiff_t>(lines.size())
                                    && contains(frame, "\033[K\n");
        const bool showsCycle = contains(lines.front(), " CICLO " + std::to_string(cycle) + " ");

        check(sameSize && redraws && clearsEachLine && showsCycle,
              "ciclo " + std::to_string(cycle) + ": 21 lineas de 80 columnas, su numero de ciclo y se redibuja en su lugar");
    }
}

void testClusterNeedles() {
    std::cout << "\n-- Agujas y valores de los relojes --\n";
    const std::string valid = render(gatewayAfter(1), ECUState::OPERATIONAL);
    check(hasBrailleIn(valid, GREEN) && !hasBrailleIn(valid, RED), "con las senales validas, las agujas van en verde");

    const std::string faults = render(gatewayAfter(8), ECUState::DEGRADED);
    check(hasBrailleIn(faults, RED) && hasBrailleIn(faults, GREEN),
          "velocidad fuera de rango: aguja roja en el tope; RPM valida: aguja verde");
    check(contains(faults, "\033[" + RED + "m-"), "el valor fuera de rango (-5) va en rojo");

    const std::string startup = render(gatewayAfter(0), ECUState::INIT);
    check(!hasBrailleIn(startup, GREEN) && !hasBrailleIn(startup, RED) && !hasBrailleIn(startup, AMBER)
              && hasBrailleIn(startup, DIM),
          "sin dato no hay aguja y el arco se apaga");

    GatewayECU notANumber;
    notANumber.processCycle({{SignalId::SPEED, std::numeric_limits<double>::quiet_NaN()}});
    const std::string nan = render(notANumber, ECUState::DEGRADED);
    check(!hasBrailleIn(nan, RED) && contains(nan, "\033[" + RED + "mn"),
          "con NaN no se dibuja aguja y el valor 'nan' va en rojo");
}

void testClusterControlBadge() {
    std::cout << "\n-- Etiqueta de la ECU de Control --\n";
    const GatewayECU gateway = gatewayAfter(1);
    check(contains(render(gateway, ECUState::INIT), "\033[1;38;5;255;48;5;240m INIT \033[0m"), "INIT en gris");
    check(contains(render(gateway, ECUState::OPERATIONAL), "\033[1;38;5;16;48;5;42m OPERATIONAL \033[0m"),
          "OPERATIONAL en verde");
    check(contains(render(gateway, ECUState::DEGRADED), "\033[1;38;5;16;48;5;214m DEGRADED \033[0m"),
          "DEGRADED en ambar");
    check(contains(render(gateway, ECUState::SAFE_STATE), "\033[1;38;5;231;48;5;196m SAFE_STATE \033[0m"),
          "SAFE_STATE en rojo");
}

void testClusterWarning() {
    std::cout << "\n-- Aviso de senales invalidas entre los relojes --\n";
    check(contains(render(gatewayAfter(1), ECUState::OPERATIONAL), "\033[" + GREEN + "m   ● EN ORDEN"),
          "0 invalidas: EN ORDEN en verde");
    check(contains(render(gatewayAfter(8), ECUState::DEGRADED), "\033[" + AMBER + "m   ▲ ATENCION"),
          "2 de 6 invalidas: ATENCION en ambar");
    check(contains(render(gatewayAfter(14), ECUState::SAFE_STATE), "\033[" + RED + "m   ▲ ATENCION"),
          "6 de 6 invalidas: ATENCION en rojo");
}

// ---------------- Valores fuera de lo común ----------------

void testUnusualValues() {
    std::cout << "\n-- Valores fuera de lo comun --\n";
    GatewayECU gateway;
    gateway.processCycle({
        {SignalId::TEMPERATURE, std::numeric_limits<double>::quiet_NaN()},
        {SignalId::THROTTLE,    1000000000.0},
    });
    const std::string frame = stripAnsi(render(gateway, ECUState::DEGRADED));

    const std::string temperature = lineStartingWith(frame, "│ TEMPERATURA");
    check(contains(temperature, "░░░░░░░░░░░░░░░░░░░░░░") && contains(temperature, "nan C")
              && contains(temperature, "FUERA DE RANGO"),
          "NaN: barra apagada, valor 'nan' y FUERA DE RANGO");

    const std::string throttle = lineStartingWith(frame, "│ ACELERADOR");
    const std::vector<std::string> lines = splitLines(frame);
    const bool sameWidth = std::all_of(lines.begin(), lines.end(), [](const std::string& line) {
        return displayWidth(line) == 80;
    });
    check(contains(throttle, "1000000000.0 %") && sameWidth,
          "un valor que no cabe en su columna se muestra completo y el marco sigue midiendo 80 columnas");
}

}  // namespace

int main() {
    std::cout << "===== Lienzo braille =====\n";
    testBrailleCanvas();

    std::cout << "\n===== Cuadros completos =====\n";
    testClusterStartup();
    testClusterWithFaults();
    testClusterBlackout();

    std::cout << "\n===== Recorrido completo =====\n";
    testClusterLayout();

    std::cout << "\n===== Agujas, aviso y estado de la ECU de Control =====\n";
    testClusterNeedles();
    testClusterControlBadge();
    testClusterWarning();

    std::cout << "\n===== Casos limite =====\n";
    testUnusualValues();

    std::cout << '\n';
    if (failures == 0) {
        std::cout << "Todas las pruebas pasaron.\n";
        return 0;
    }
    std::cout << failures << " prueba(s) fallaron.\n";
    return 1;
}

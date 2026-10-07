#include "Dashboard.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "BrailleCanvas.hpp"

namespace {

const std::string TITLE = "ECU GATEWAY / CONTROL";
const std::string NO_VALUE = "---";

std::string formatNumber(double value, int decimals) {
    std::ostringstream text;
    text << std::fixed << std::setprecision(decimals) << value;
    return text.str();
}

// Sin dato no hay valor confiable que mostrar, ni el 0.0 inicial ni el último recibido
std::string formatValue(const Sensor& sensor, int decimals) {
    if (sensor.getState() == SignalState::NOT_AVAILABLE) {
        return NO_VALUE;
    }
    return formatNumber(sensor.getValue(), decimals);
}

std::string signalStateText(SignalState state) {
    switch (state) {
        case SignalState::VALID:         return "VALIDA";
        case SignalState::OUT_OF_RANGE:  return "FUERA DE RANGO";
        case SignalState::NOT_AVAILABLE: return "NO DISPONIBLE";
    }
    return "DESCONOCIDO";
}

std::string controlStateText(ECUState state) {
    switch (state) {
        case ECUState::INIT:        return "INIT";
        case ECUState::OPERATIONAL: return "OPERATIONAL";
        case ECUState::DEGRADED:    return "DEGRADED";
        case ECUState::SAFE_STATE:  return "SAFE_STATE";
    }
    return "DESCONOCIDO";
}

// Vacío mientras la señal no lleve ciclos sin dato
std::string formatMissedCycles(unsigned int missedCycles) {
    if (missedCycles == 0) {
        return "";
    }
    return std::to_string(missedCycles) + (missedCycles == 1 ? " ciclo" : " ciclos");
}

// Secuencias ANSI, se reescribe encima del cuadro anterior para que no parpadee
const std::string CURSOR_HOME    = "\033[H";
const std::string CLEAR_LINE_END = "\033[K";
const std::string CLEAR_BELOW    = "\033[J";
const std::string RESET          = "\033[0m";

// Colores de 256 tonos (parámetros SGR)
const std::string FRAME_COLOR = "38;5;240";
const std::string LABEL_COLOR = "38;5;250";
const std::string SCALE_COLOR = "38;5;244";
const std::string DIM_COLOR   = "38;5;238";
const std::string VALUE_COLOR = "1;38;5;255";
const std::string GREEN       = "38;5;42";
const std::string AMBER       = "38;5;214";
const std::string RED         = "1;38;5;196";

constexpr std::size_t CLUSTER_WIDTH = 78;  // Ancho interior del marco
constexpr std::size_t CENTER_WIDTH = 16;   // Columna entre los dos relojes
constexpr std::size_t GAUGE_NAME_WIDTH = 14;
constexpr std::size_t GAUGE_BAR_WIDTH = 22;
constexpr std::size_t GAUGE_VALUE_WIDTH = 6;
constexpr std::size_t GAUGE_UNIT_WIDTH = 4;

constexpr int DIAL_COLUMNS = 30;
constexpr int DIAL_ROWS = 9;
constexpr double DIAL_SWEEP_DEGREES = 220.0;
constexpr double PI = 3.14159265358979323846;

std::string stateColor(SignalState state) {
    switch (state) {
        case SignalState::VALID:         return GREEN;
        case SignalState::OUT_OF_RANGE:  return RED;
        case SignalState::NOT_AVAILABLE: return AMBER;
    }
    return "";
}

// Etiqueta con fondo de color para el estado de la ECU de Control
std::string controlBadgeColor(ECUState state) {
    switch (state) {
        case ECUState::INIT:        return "1;38;5;255;48;5;240";
        case ECUState::OPERATIONAL: return "1;38;5;16;48;5;42";
        case ECUState::DEGRADED:    return "1;38;5;16;48;5;214";
        case ECUState::SAFE_STATE:  return "1;38;5;231;48;5;196";
    }
    return "";
}

// Nombre de la señal bajo su barra
std::string gaugeName(SignalId id) {
    switch (id) {
        case SignalId::SPEED:           return "VELOCIDAD";
        case SignalId::RPM:             return "RPM";
        case SignalId::TEMPERATURE:     return "TEMPERATURA";
        case SignalId::THROTTLE:        return "ACELERADOR";
        case SignalId::BATTERY_VOLTAGE: return "BATERIA";
        case SignalId::OIL_PRESSURE:    return "ACEITE";
    }
    return "DESCONOCIDA";
}

// Ancho en columnas, los caracteres de cuadro, bloque y braille ocupan 3 bytes y una columna
std::size_t displayWidth(const std::string& text) {
    return static_cast<std::size_t>(std::count_if(text.begin(), text.end(), [](char byte) {
        return (static_cast<unsigned char>(byte) & 0xC0) != 0x80;
    }));
}

std::string repeat(const std::string& piece, std::size_t times) {
    std::string text;
    for (std::size_t index = 0; index < times; ++index) {
        text += piece;
    }
    return text;
}

std::string centered(const std::string& text, std::size_t width) {
    const std::size_t textWidth = displayWidth(text);
    if (textWidth >= width) {
        return text;
    }
    const std::size_t left = (width - textWidth) / 2;
    return std::string(left, ' ') + text + std::string(width - textWidth - left, ' ');
}

// Línea de texto con tramos de color, lleva la cuenta de su ancho visible
class StyledLine {
public:
    StyledLine& add(const std::string& text, const std::string& color = "") {
        spans.push_back({text, color});
        visibleWidth += displayWidth(text);
        return *this;
    }

    StyledLine& padTo(std::size_t width) {
        if (visibleWidth < width) {
            add(std::string(width - visibleWidth, ' '));
        }
        return *this;
    }

    StyledLine& append(const StyledLine& other) {
        for (const Span& span : other.spans) {
            add(span.text, span.color);
        }
        return *this;
    }

    std::string render() const {
        std::string text;
        for (const Span& span : spans) {
            text += span.color.empty() ? span.text : "\033[" + span.color + "m" + span.text + RESET;
        }
        return text;
    }

private:
    struct Span {
        std::string text;
        std::string color;
    };
    std::vector<Span> spans;
    std::size_t visibleWidth{0};
};

// Posición en la escala, de 0 a 1. Vacía si no hay aguja que dibujar (sin dato o NaN)
std::optional<double> scaleFraction(const Sensor& sensor) {
    if (sensor.getState() == SignalState::NOT_AVAILABLE || std::isnan(sensor.getValue())) {
        return std::nullopt;
    }
    const double fraction = (sensor.getValue() - sensor.getMin()) / (sensor.getMax() - sensor.getMin());
    // Fuera de rango, la aguja se queda en el tope
    return std::clamp(fraction, 0.0, 1.0);
}

// Reloj con aguja, arco de la escala, marcas, números, aguja y el valor al centro
std::vector<StyledLine> drawDial(const Sensor& sensor, int labelCount, double labelDivisor) {
    struct Cell {
        std::string glyph{" "};
        std::string color;
    };

    // Medidas en puntos braille, cada celda tiene 2 de ancho y 4 de alto
    const double centerX = DIAL_COLUMNS;
    const double centerY = 4 * DIAL_ROWS * 0.60;
    const double radius = std::min(DIAL_COLUMNS - 2.0, centerY - 1);
    const double startDegrees = 90 + DIAL_SWEEP_DEGREES / 2;
    const auto angleAt = [&](double fraction) {
        return (startDegrees - DIAL_SWEEP_DEGREES * fraction) * (PI / 180.0);
    };
    const auto labelFraction = [labelCount](int label) {
        return static_cast<double>(label) / (labelCount - 1);
    };

    BrailleCanvas arc(DIAL_COLUMNS, DIAL_ROWS);
    for (int step = 0; step <= 720; ++step) {
        const double angle = angleAt(step / 720.0);
        arc.setDot(centerX + radius * std::cos(angle), centerY - radius * std::sin(angle));
    }
    for (int label = 0; label < labelCount; ++label) {
        const double angle = angleAt(labelFraction(label));
        arc.drawLine(centerX + (radius - 3) * std::cos(angle), centerY - (radius - 3) * std::sin(angle),
                     centerX + radius * std::cos(angle), centerY - radius * std::sin(angle));
    }

    BrailleCanvas needle(DIAL_COLUMNS, DIAL_ROWS);
    const std::optional<double> fraction = scaleFraction(sensor);
    if (fraction.has_value()) {
        const double angle = angleAt(*fraction);
        needle.drawLine(centerX, centerY,
                        centerX + (radius - 6) * std::cos(angle), centerY - (radius - 6) * std::sin(angle));
    }

    // La aguja va encima del arco, un arco sin dato se apaga
    const std::string signalColor = stateColor(sensor.getState());
    const std::string arcColor = sensor.getState() == SignalState::NOT_AVAILABLE ? DIM_COLOR : SCALE_COLOR;
    std::vector<std::vector<Cell>> cells(DIAL_ROWS, std::vector<Cell>(DIAL_COLUMNS));
    for (int row = 0; row < DIAL_ROWS; ++row) {
        for (int column = 0; column < DIAL_COLUMNS; ++column) {
            const unsigned char arcDots = arc.dots(column, row);
            const unsigned char needleDots = needle.dots(column, row);
            Cell& cell = cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
            if (needleDots != 0) {
                cell = {BrailleCanvas::glyph(static_cast<unsigned char>(needleDots | arcDots)), signalColor};
            } else if (arcDots != 0) {
                cell = {BrailleCanvas::glyph(arcDots), arcColor};
            }
        }
    }

    // El texto va encima de todo
    const auto writeText = [&cells](int row, int column, const std::string& text, const std::string& color) {
        for (std::size_t index = 0; index < text.size(); ++index) {
            const int target = column + static_cast<int>(index);
            if (row >= 0 && row < DIAL_ROWS && target >= 0 && target < DIAL_COLUMNS) {
                cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(target)] = {std::string(1, text[index]), color};
            }
        }
    };

    for (int label = 0; label < labelCount; ++label) {
        const double angle = angleAt(labelFraction(label));
        const double labelValue =
            (sensor.getMin() + (sensor.getMax() - sensor.getMin()) * labelFraction(label)) / labelDivisor;
        const std::string text = std::to_string(std::lround(labelValue));
        const double dotX = centerX + (radius - 9) * std::cos(angle);
        const double dotY = centerY - (radius - 9) * std::sin(angle);
        writeText(static_cast<int>(dotY / 4), static_cast<int>(dotX / 2) - static_cast<int>(text.size()) / 2, text,
                  SCALE_COLOR);
    }

    const std::string value = formatValue(sensor, 0);
    const std::string unit = sensor.getUnit();
    writeText(DIAL_ROWS - 2, DIAL_COLUMNS / 2 - static_cast<int>(value.size()) / 2, value,
              sensor.getState() == SignalState::VALID ? VALUE_COLOR : signalColor);
    writeText(DIAL_ROWS - 1, DIAL_COLUMNS / 2 - static_cast<int>(unit.size()) / 2, unit, LABEL_COLOR);

    std::vector<StyledLine> lines;
    for (const std::vector<Cell>& rowCells : cells) {
        StyledLine line;
        for (const Cell& cell : rowCells) {
            line.add(cell.glyph, cell.glyph == " " ? "" : cell.color);
        }
        lines.push_back(line);
    }
    return lines;
}

// Barra de 1/8 de celda de resolución, sin dato queda apagada
StyledLine drawBar(const Sensor& sensor, std::size_t width) {
    StyledLine line;
    const std::optional<double> fraction = scaleFraction(sensor);
    if (!fraction.has_value()) {
        return line.add(repeat("░", width), DIM_COLOR);
    }

    const std::vector<std::string> eighths = {"", "▏", "▎", "▍", "▌", "▋", "▊", "▉"};
    const double cellsFilled = *fraction * static_cast<double>(width);
    // Nunca más celdas que el ancho, aunque la fracción viniera mayor que 1
    const auto fullCells = std::min(static_cast<std::size_t>(cellsFilled), width);
    const auto eighth = static_cast<std::size_t>((cellsFilled - static_cast<double>(fullCells)) * 8);
    const bool partial = eighth > 0 && fullCells < width;

    line.add(repeat("█", fullCells) + (partial ? eighths[eighth] : ""), stateColor(sensor.getState()));
    return line.add(repeat("░", width - fullCells - (partial ? 1 : 0)), DIM_COLOR);
}

StyledLine drawGauge(const Sensor& sensor) {
    const unsigned int missed = sensor.getMissedCycles();
    StyledLine line;
    line.add(" ").add(gaugeName(sensor.getId()), LABEL_COLOR).padTo(1 + GAUGE_NAME_WIDTH);
    line.append(drawBar(sensor, GAUGE_BAR_WIDTH)).add("  ");

    const std::string value = formatValue(sensor, 1);
    line.add(std::string(GAUGE_VALUE_WIDTH - std::min(GAUGE_VALUE_WIDTH, value.size()), ' ') + value, VALUE_COLOR);

    std::string unit = " " + sensor.getUnit();
    unit += std::string(1 + GAUGE_UNIT_WIDTH - std::min(1 + GAUGE_UNIT_WIDTH, unit.size()), ' ');
    line.add(unit, LABEL_COLOR).add(signalStateText(sensor.getState()), stateColor(sensor.getState()));
    if (missed > 0) {
        line.add(" · " + formatMissedCycles(missed), SCALE_COLOR);
    }
    return line;
}

// Aviso entre los dos relojes con cuántas señales están inválidas
std::vector<StyledLine> drawWarning(std::size_t invalid, std::size_t total) {
    const std::string color = invalid == 0 ? GREEN : invalid < total ? AMBER : RED;
    std::vector<StyledLine> lines(static_cast<std::size_t>(DIAL_ROWS));
    lines[3].add(centered(invalid == 0 ? "● EN ORDEN" : "▲ ATENCION", CENTER_WIDTH), color);
    lines[5].add(centered(std::to_string(invalid) + " de " + std::to_string(total), CENTER_WIDTH), color);
    lines[6].add(centered("invalidas", CENTER_WIDTH), SCALE_COLOR);
    for (StyledLine& line : lines) {
        line.padTo(CENTER_WIDTH);
    }
    return lines;
}

}  // namespace

Dashboard::Dashboard(std::ostream& output) : out(output) {
}

void Dashboard::render(const GatewayECU& gateway, ECUState controlState) const {
    const Sensor& speed = gateway.findSensor(SignalId::SPEED);
    const Sensor& rpm = gateway.findSensor(SignalId::RPM);
    const std::vector<StyledLine> speedDial = drawDial(speed, 6, 1.0);
    const std::vector<StyledLine> rpmDial = drawDial(rpm, 9, 1000.0);
    const std::vector<StyledLine> warning = drawWarning(gateway.countInvalidSignals(), gateway.getSensors().size());

    std::vector<StyledLine> body(1);
    for (std::size_t row = 0; row < speedDial.size(); ++row) {
        body.push_back(StyledLine().add(" ").append(speedDial[row]).append(warning[row]).append(rpmDial[row]));
    }
    const std::string gap(CENTER_WIDTH, ' ');
    const auto dialWidth = static_cast<std::size_t>(DIAL_COLUMNS);
    body.push_back(StyledLine().add(" ").add(centered("VELOCIDAD", dialWidth), LABEL_COLOR).add(gap)
                       .add(centered("RPM x1000", dialWidth), LABEL_COLOR));
    body.push_back(StyledLine().add(" ")
                       .add(centered(signalStateText(speed.getState()), dialWidth), stateColor(speed.getState()))
                       .add(gap)
                       .add(centered(signalStateText(rpm.getState()), dialWidth), stateColor(rpm.getState())));
    body.emplace_back();

    const std::vector<SignalId> gauges = {
        SignalId::TEMPERATURE, SignalId::THROTTLE, SignalId::BATTERY_VOLTAGE, SignalId::OIL_PRESSURE,
    };
    std::for_each(gauges.begin(), gauges.end(),
                  [&](SignalId id) { body.push_back(drawGauge(gateway.findSensor(id))); });

    const std::string controlText = " " + controlStateText(controlState) + " ";
    const std::string controlLabel = "ECU DE CONTROL  ";
    StyledLine footer;
    footer.padTo(CLUSTER_WIDTH - displayWidth(controlText) - displayWidth(controlLabel) - 2)
        .add(controlLabel, LABEL_COLOR)
        .add(controlText, controlBadgeColor(controlState));

    const std::string cycleText = "CICLO " + std::to_string(gateway.getCycleCount());
    const std::string frame = "\033[" + FRAME_COLOR + "m";
    const std::string title = "\033[" + VALUE_COLOR + "m";

    out << CURSOR_HOME;
    out << frame << "╭─ " << RESET << title << TITLE << RESET << frame << ' '
        << repeat("─", CLUSTER_WIDTH - TITLE.size() - cycleText.size() - 6) << ' ' << RESET << title << cycleText
        << RESET << frame << " ─╮" << RESET << CLEAR_LINE_END << '\n';
    const auto writeRow = [&](StyledLine& line) {
        line.padTo(CLUSTER_WIDTH);
        out << frame << "│" << RESET << line.render() << frame << "│" << RESET << CLEAR_LINE_END << '\n';
    };
    std::for_each(body.begin(), body.end(), writeRow);
    out << frame << "├" << repeat("─", CLUSTER_WIDTH) << "┤" << RESET << CLEAR_LINE_END << '\n';
    writeRow(footer);
    out << frame << "╰" << repeat("─", CLUSTER_WIDTH) << "╯" << RESET << CLEAR_LINE_END << '\n';
    out << CLEAR_BELOW;

    // Que el cuadro aparezca completo antes de que main espere el siguiente ciclo
    out.flush();
}

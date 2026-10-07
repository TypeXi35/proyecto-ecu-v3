#include "BrailleCanvas.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace {

// Bit de cada punto dentro de su celda, indexado por [fila][columna] del punto
constexpr std::array<std::array<unsigned char, 2>, 4> DOT_BITS = {{
    {{0x01, 0x08}},
    {{0x02, 0x10}},
    {{0x04, 0x20}},
    {{0x40, 0x80}},
}};

int nearest(double value) {
    return static_cast<int>(std::floor(value + 0.5));
}

}  // namespace

BrailleCanvas::BrailleCanvas(int columns, int rows)
    : columnCount(columns),
      rowCount(rows),
      cells(static_cast<std::size_t>(columns * rows), 0) {
}

void BrailleCanvas::setDot(double x, double y) {
    const int dotX = nearest(x);
    const int dotY = nearest(y);
    if (dotX < 0 || dotY < 0 || dotX >= 2 * columnCount || dotY >= 4 * rowCount) {
        return;
    }
    const auto index = static_cast<std::size_t>((dotY / 4) * columnCount + dotX / 2);
    const unsigned char bit = DOT_BITS[static_cast<std::size_t>(dotY % 4)][static_cast<std::size_t>(dotX % 2)];
    cells[index] = static_cast<unsigned char>(cells[index] | bit);
}

void BrailleCanvas::drawLine(double x0, double y0, double x1, double y1) {
    // Dos pasos por punto de distancia para no dejar huecos
    const int steps = static_cast<int>(std::max(std::abs(x1 - x0), std::abs(y1 - y0)) * 2) + 1;
    for (int step = 0; step <= steps; ++step) {
        const double t = static_cast<double>(step) / steps;
        setDot(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t);
    }
}

unsigned char BrailleCanvas::dots(int column, int row) const {
    if (column < 0 || row < 0 || column >= columnCount || row >= rowCount) {
        return 0;
    }
    return cells[static_cast<std::size_t>(row * columnCount + column)];
}

int BrailleCanvas::columns() const {
    return columnCount;
}

int BrailleCanvas::rows() const {
    return rowCount;
}

std::string BrailleCanvas::glyph(unsigned char dots) {
    // Los caracteres braille van de U+2800 a U+28FF; en UTF-8 son 3 bytes
    std::string text(3, '\0');
    text[0] = static_cast<char>(0xE2);
    text[1] = static_cast<char>(0xA0 + (dots >> 6));
    text[2] = static_cast<char>(0x80 + (dots & 0x3F));
    return text;
}

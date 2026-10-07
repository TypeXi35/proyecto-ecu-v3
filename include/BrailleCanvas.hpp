#pragma once

#include <string>
#include <vector>

// Lienzo de puntos para dibujar en terminal, cada celda es un carácter braille de 2 x 4 puntos
class BrailleCanvas {
public:
    BrailleCanvas(int columns, int rows);

    // Enciende el punto más cercano a (x, y); fuera del lienzo no hace nada
    void setDot(double x, double y);

    // Enciende los puntos de la recta entre (x0, y0) y (x1, y1)
    void drawLine(double x0, double y0, double x1, double y1);

    // Puntos encendidos de una celda, como bits del patrón braille
    unsigned char dots(int column, int row) const;

    int columns() const;
    int rows() const;

    // Carácter braille en UTF-8 para un patrón de puntos
    static std::string glyph(unsigned char dots);

private:
    int columnCount;
    int rowCount;
    std::vector<unsigned char> cells;
};

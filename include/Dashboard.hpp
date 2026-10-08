#pragma once

#include <iostream>
#include <ostream>

#include "ControlData.hpp"
#include "ECUState.hpp"

// Tablero de instrumentos en terminal, muestra el estado de las señales y el de la ECU de Control
// No decide ningún estado
class Dashboard {
public:
    // Dibuja en la pantalla, las pruebas le pasan otro destino
    explicit Dashboard(const ECUData& sourceData, std::ostream& output = std::cout);

    // Dibuja un cuadro con relojes para velocidad y RPM
    void render() const;

private:
    const ECUData& controlData;
    std::ostream& out;
};

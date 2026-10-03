#pragma once

#include <vector>

#include "Sensor.hpp"
#include "SignalTypes.hpp"

// Recibe las señales y determina si son confiables
class GatewayECU
{
public:
    // Crea los sensores con sus rangos válidos
    GatewayECU();

    // Actualiza los sensores con lectura y registra ciclo sin dato en los demás
    void processCycle(const std::vector<SignalReading> &readings);

    // Acceso de solo lectura a los sensores
    const std::vector<Sensor> &getSensors() const;

private:
    // La Gateway es dueña de los sensores
    std::vector<Sensor> sensors;
};

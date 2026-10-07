#pragma once

#include <cstddef>
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

    // Busca un sensor por ID, si no existe da std::out_of_range
    const Sensor &findSensor(SignalId id) const;

    // Cuenta las señales que no están en VALID
    std::size_t countInvalidSignals() const;

    // Ciclos procesados desde que se creó la Gateway
    unsigned int getCycleCount() const;

private:
    // La Gateway es dueña de los sensores
    std::vector<Sensor> sensors;
    unsigned int cycleCount{0};
};

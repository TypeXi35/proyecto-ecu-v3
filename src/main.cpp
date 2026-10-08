#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "ControlECU.hpp"
#include "Dashboard.hpp"
#include "GatewayECU.hpp"
#include "VehicleSimulator.hpp"

#ifdef _WIN32
#include <windows.h>
#endif
// Pausa entre ciclos para que el tablero se alcance a leer
constexpr std::chrono::milliseconds CYCLE_PERIOD{500};

int main()
{
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #endif
    VehicleSimulator simulator;
    GatewayECU gateway;
    ControlECU control(gateway.getSensors());
    Dashboard dashboard(control.getControlData());

    // Ciclo 0: todavía no llega ninguna señal y la Control está en INIT
    dashboard.render();
    std::this_thread::sleep_for(CYCLE_PERIOD);

    // Corre hasta que la Control llega a SAFE_STATE; el último cuadro ya muestra ese estado
    while (control.getControlData().currentState != ECUState::SAFE_STATE)
    {
        simulator.updateSignal();
        gateway.processCycle(simulator.exposeSignals());
        control.runControlCycle();
        dashboard.render();
        std::this_thread::sleep_for(CYCLE_PERIOD);
    }

    std::cout << "ECU de Control en SAFE_STATE, simulación detenida\n";
    return EXIT_FAILURE;
}

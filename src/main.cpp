#include <chrono>
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
    Dashboard dashboard;

    // Ciclo 0: todavía no llega ninguna señal y la Control está en INIT
    dashboard.render(gateway, control.getControlData().currentState);
    std::this_thread::sleep_for(CYCLE_PERIOD);

    // No se detiene en SAFE_STATE: las señales siguen llegando y la Control permanece ahí hasta Ctrl+C
    while (true)
    {
        simulator.updateSignal();
        gateway.processCycle(simulator.exposeSignals());
        control.runControlCycle();
        dashboard.render(gateway, control.getControlData().currentState);
        std::this_thread::sleep_for(CYCLE_PERIOD);
    }

    return 0;
}

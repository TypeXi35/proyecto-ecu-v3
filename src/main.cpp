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
    VehicleSimulator simulator;
    GatewayECU gateway;
    ControlECU control;
    Dashboard dashboard;
    // Ciclo 0: todavía no llega ninguna señal y la Control está en INIT
    dashboard.postUpdate(control.getControlData());

    std::thread worker([&simulator, &gateway, &control, &dashboard] {
        // Corre hasta que la Control llega a SHUTDOWN
        while (control.getControlData().currentState != ECUState::SHUTDOWN) {
            simulator.updateSignal();
            gateway.processCycle(simulator.exposeSignals());
            control.runControlCycle(gateway.getSensors());

            dashboard.postUpdate(control.getControlData());

            std::this_thread::sleep_for(CYCLE_PERIOD);
        }
    });

    dashboard.renderLoop();

    if (worker.joinable()) {worker.join();}

    std::cout << "ECU de Control en SHUTDOWN, simulación detenida\n";
    return EXIT_SUCCESS;
}

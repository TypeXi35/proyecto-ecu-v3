#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "ControlECU.hpp"
#include "Dashboard.hpp"
#include "GatewayECU.hpp"
#include "CSVLogger.hpp"
#include "VehicleSimulator.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

// dotenv.h usa std::getenv, que MSVC marca como inseguro (C4996)
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
#include "dotenv.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
// Pausa entre ciclos para que el tablero se alcance a leer
constexpr std::chrono::milliseconds CYCLE_PERIOD{500};

int main()
{
    VehicleSimulator simulator;
    GatewayECU gateway;
    ControlECU control;
    Dashboard dashboard;
    // Carga el .env del directorio de trabajo; Preserve deja ganar a las variables ya definidas
    dotenv::init(dotenv::Preserve, ".env");
    const std::string loggerType = dotenv::getenv("LOGGER_TYPE", "CSV");
    const std::string logFile    = dotenv::getenv("LOG_FILE", "ecu_log.csv");

    std::unique_ptr<Logger> logger;
    if (loggerType == "BINARY") {
        // TODO: logger = std::make_unique<BinaryLogger>(logFile);
        std::cerr << "LOGGER_TYPE=BINARY todavía no está implementado\n";
        return EXIT_FAILURE;
    } else {
        logger = std::make_unique<CSVLogger>(logFile);
    }
    // Ciclo 0: todavía no llega ninguna señal y la Control está en INIT
    dashboard.postUpdate(control.getControlData());

    std::thread worker([&simulator, &gateway, &control, &dashboard, &logger] {
        // Corre hasta que la Control llega a SHUTDOWN
        while (control.getControlData().currentState != ECUState::SHUTDOWN) {
            simulator.updateSignal();
            gateway.processCycle(simulator.exposeSignals());
            control.runControlCycle(gateway.getSensors());
            (*logger)(control.getControlData());

            dashboard.postUpdate(control.getControlData());

            std::this_thread::sleep_for(CYCLE_PERIOD);
        }
    });

    dashboard.renderLoop();

    if (worker.joinable()) {worker.join();}

    std::cout << "ECU de Control en SHUTDOWN, simulación detenida\n";
    return EXIT_SUCCESS;
}

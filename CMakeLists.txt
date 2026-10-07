cmake_minimum_required(VERSION 3.16)

project(
    AutomotiveECU
    
    VERSION 1.0
    DESCRIPTION "Automotive ECU Simulation System"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
    ecu_core

    src/BrailleCanvas.cpp
    src/ControlECU.cpp
    src/Dashboard.cpp
    src/GatewayECU.cpp
    src/Sensor.cpp
    src/VehicleSimulator.cpp
)

target_include_directories(
    ecu_core

    PUBLIC

    ${PROJECT_SOURCE_DIR}/include
)

if(MSVC)

    target_compile_options(
        ecu_core

        PRIVATE

        /W4
    )

else()

    target_compile_options(
        ecu_core

        PRIVATE

        -Wall
        -Wextra
        -Wpedantic
    )
    
endif()

add_executable(
    ecu_simulator

    src/main.cpp
)

target_link_libraries(
    ecu_simulator

    PRIVATE

    ecu_core
)
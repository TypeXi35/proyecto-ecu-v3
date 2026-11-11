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

# Pruebas de la Gateway y del Dashboard con un guion de datos fijos
add_executable(
    gateway_tests

    tests/gateway_tests.cpp
)

target_link_libraries(
    gateway_tests

    PRIVATE

    ecu_core
)

add_executable(
    dashboard_tests

    tests/dashboard_tests.cpp
)

target_link_libraries(
    dashboard_tests

    PRIVATE

    ecu_core
)

# ---------------- Configuracion de GoogleTest ----------------
#Activar CTest
enable_testing()

#Descarga GoogleTest y lo compila con el proyecto
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

#Ejecutable de pruebas de la ECU de Control
add_executable(
    control_tests
    tests/control_tests.cpp
)

target_link_libraries(
    control_tests
    PRIVATE
    ecu_core
    GTest::gtest_main
)

# Registrar TEST() en CTest

include(GoogleTest)
gtest_discover_tests(control_tests)
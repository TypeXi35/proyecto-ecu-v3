cmake_minimum_required(VERSION 3.16)

project(
    AutomotiveECU
    
    VERSION 1.0
    DESCRIPTION "Automotive ECU Simulation System"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_FIND_DEBUG_MODE ON)
include(FetchContent)

### FTXUI(https://github.com/ArthurSonzogni/ftxui-starter/blob/52af91f09891444e86f3974cb8d6e195d3e10653/CMakeLists.txt#L13)
# try to use distro's ftxui :)))
find_package(ftxui)

if (NOT ftxui_FOUND)
    # fallback to FetchContent
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
    set(FETCHCONTENT_QUIET OFF)

    FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
        GIT_TAG        v7.0.3
        GIT_PROGRESS   TRUE
        GIT_SHALLOW    TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(ftxui)
endif()
###

### RapidCSV (https://github.com/d99kris/rapidcsv) - header-only CSV parser
FetchContent_Declare(rapidcsv
    GIT_REPOSITORY https://github.com/d99kris/rapidcsv.git
    GIT_TAG        v9.09
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)
set(RAPIDCSV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rapidcsv)
###

### dotenv-cpp (https://github.com/laserpants/dotenv-cpp) - header-only .env loader
# No release tags upstream, so it is pinned to a commit.
FetchContent_Declare(dotenv
    GIT_REPOSITORY https://github.com/laserpants/dotenv-cpp.git
    GIT_TAG        9275210b8abf1a551fb81e0ba45866286d764acd
    EXCLUDE_FROM_ALL
)
set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(dotenv)
###

add_library(
    ecu_core

    src/BrailleCanvas.cpp
    src/ControlECU.cpp
    src/CSVLogger.cpp
    src/Dashboard.cpp
    src/GatewayECU.cpp
    src/Logger.cpp
    src/Sensor.cpp
    src/VehicleSimulator.cpp
)
target_link_libraries(
    ecu_core PUBLIC
    ftxui::screen
    ftxui::dom
    ftxui::component
    rapidcsv
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
    dotenv
)

# ---------------- Configuracion de GoogleTest ----------------
#Activar CTest
enable_testing()

#Descarga GoogleTest y lo compila con el proyecto
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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


# ---------------- VehicleSimulator Google Test ----------------

# Ejecutable de pruebas VehicleSimulator.
add_executable(
    vehicleSimulator_test
    tests/vehicleSimulator_test.cpp
)

# Vincula el proyecto y las bibliotecas de Google Test.
target_link_libraries(
    vehicleSimulator_test
    PRIVATE
    ecu_core
    GTest::gtest_main
)

# Registrar las pruebas de VehicleSimulator en CTest.
gtest_discover_tests(vehicleSimulator_test)

# ---------------- GatewayECU Google Test ----------------

# Ejecutable de pruebas de la GatewayECU.
add_executable(
    gateway_test
    tests/gateway_test.cpp
)

# Vincula el proyecto y las bibliotecas de Google Test.
target_link_libraries(
    gateway_test
    PRIVATE
    ecu_core
    GTest::gtest_main
)

# Registrar las pruebas de GatewayECU en CTest.
gtest_discover_tests(gateway_test)

# ---------------- CSVLogger Google Test ----------------

# Ejecutable de pruebas del CSVLogger.
add_executable(
    csvLogger_test
    tests/csvLogger_test.cpp
)

# Vincula el proyecto y las bibliotecas de Google Test.
target_link_libraries(
    csvLogger_test
    PRIVATE
    ecu_core
    GTest::gtest_main
)

# Registrar las pruebas del CSVLogger en CTest.
gtest_discover_tests(csvLogger_test)
#include "VehicleSimulator.hpp"

VehicleSimulator::VehicleSimulator()
    : 
    generator(12345),
    signalNoise(0.0, 1.0),
    speed(0.0),
    rpm(800.0),
    temperature(25.0),
    batteryVoltage(12.6),
    oilPressure(1.5),
    throttle(0.0)
{
}

void VehicleSimulator::updateSignal()
{
    // Throttle
    std::uniform_real_distribution<double> throttleChange(-8.0, 8.0);

    throttle += throttleChange(generator);

    throttle = std::clamp(throttle, 0.0, 100.0);

    // Speed
    double targetSpeed = throttle * 1.8;

    speed += (targetSpeed - speed) * 0.05;

    speed = std::max(0.0, speed);

    // RPM
    rpm = 800.0 + speed * 22.0 + throttle * 18.0 + signalNoise(generator) * 50.0;

    rpm = std::clamp(rpm, 700.0, 7000.0);

    // Temperature
    double targetTemperature = 85.0 + throttle * 0.08;

    temperature += (targetTemperature - temperature) * 0.01;

    temperature += signalNoise(generator) * 0.05;

    // Battery Voltage
    batteryVoltage = 13.8 + signalNoise(generator) * 0.08;

    // Oil Pressure
    oilPressure = 1.0 + rpm / 2000.0 + signalNoise(generator) * 0.05;

    oilPressure = std::max(0.0, oilPressure);
}

std::vector<SignalVehicle> VehicleSimulator::exposeSignals() const
{
    std::vector<SignalVehicle> signals;

    signals.push_back({"Throttle", throttle});
    signals.push_back({"Speed", speed});
    signals.push_back({"RPM", rpm});
    signals.push_back({"Temperature", temperature});
    signals.push_back({"Battery Voltage", batteryVoltage});
    signals.push_back({"Oil Pressure", oilPressure});

    return signals;
}

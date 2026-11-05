
#include "VehicleSimulator.hpp"

#include <algorithm>

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
    // Update throttle with a random variation.
    std::uniform_real_distribution<double> throttleChange(-8.0, 8.0);

    throttle += throttleChange(generator);
    throttle = std::clamp(throttle, 0.0, 100.0);

    // Adjust speed gradually toward the target speed.
    double targetSpeed = throttle * 1.8;

    speed += (targetSpeed - speed) * 0.05;
    speed = std::max(0.0, speed);

    // Calculate RPM based on speed, throttle and noise.
    rpm = 800.0 + speed * 22.0 + throttle * 18.0 + signalNoise(generator) * 50.0;

    rpm = std::clamp(rpm, 700.0, 7000.0);

    // Adjust temperature gradually toward its target.
    double targetTemperature = 85.0 + throttle * 0.08;

    temperature += (targetTemperature - temperature) * 0.01;
    temperature += signalNoise(generator) * 0.05;

    // Simulate battery voltage fluctuations.
    batteryVoltage = 13.8 + signalNoise(generator) * 0.08;

    // Calculate oil pressure based on RPM and noise.
    oilPressure = 1.0 + rpm / 2000.0 + signalNoise(generator) * 0.05;

    oilPressure = std::max(0.0, oilPressure);

    // Update fault states after the physical values.
    updateFaults();
}

void VehicleSimulator::updateFaults()
{
    std::vector<SignalId> recoveredSignals;

    for (SignalFault& fault : faults)
    {
        if (fault.type != FaultType::NONE)
        {
            if (fault.remainingCycles > 0)
            {
                fault.remainingCycles--;
            }

            if (fault.remainingCycles == 0)
            {
                fault.type = FaultType::NONE;
                recoveredSignals.push_back(fault.id);
            }
        }
    }

    std::uniform_real_distribution<double> faultChance(0.0, 1.0);

    double chance = faultChance(generator);

    if (chance >= 0.05)
    {
        return;
    }

    std::uniform_int_distribution<int> signalChoice(0, 5);

    int selectedSignal = signalChoice(generator);

    SignalId selectedId;

    switch (selectedSignal)
    {
        case 0:
            selectedId = SignalId::SPEED;
            break;

        case 1:
            selectedId = SignalId::RPM;
            break;

        case 2:
            selectedId = SignalId::TEMPERATURE;
            break;

        case 3:
            selectedId = SignalId::THROTTLE;
            break;

        case 4:
            selectedId = SignalId::BATTERY_VOLTAGE;
            break;

        case 5:
            selectedId = SignalId::OIL_PRESSURE;
            break;
    }

    bool justRecovered =
        std::find(
            recoveredSignals.begin(),
            recoveredSignals.end(),
            selectedId) != recoveredSignals.end();

    if (justRecovered)
    {
        return;
    }

    if (hasActiveFault(selectedId))
    {
        return;
    }

    std::uniform_int_distribution<int> faultChoice(0, 1);

    int selectedFault = faultChoice(generator);

    FaultType selectedType;

    if (selectedFault == 0)
    {
        selectedType = FaultType::OUT_OF_RANGE;
    }
    else
    {
        selectedType = FaultType::MISSING;
    }

    std::uniform_int_distribution<unsigned int> faultDuration(3, 10);

    unsigned int duration = faultDuration(generator);

    activateFault(selectedId, selectedType, duration);
}

void VehicleSimulator::activateFault(
    SignalId id,
    FaultType type,
    unsigned int duration)
{
    if (type == FaultType::NONE || duration == 0)
    {
        return;
    }

    auto foundFault = std::find_if(
        faults.begin(),
        faults.end(),
        [id](const SignalFault& fault)
        {
            return fault.id == id;
        }
    );

    if (foundFault == faults.end())
    {
        faults.push_back({id, type, duration});
        return;
    }

    if (foundFault->type != FaultType::NONE)
    {
        return;
    }

    foundFault->type = type;
    foundFault->remainingCycles = duration;
}

bool VehicleSimulator::hasActiveFault(SignalId id) const
{
    return getFaultType(id) != FaultType::NONE;
}

FaultType VehicleSimulator::getFaultType(SignalId id) const
{
    auto foundFault = std::find_if(
        faults.begin(),
        faults.end(),
        [id](const SignalFault& fault)
        {
            return fault.id == id &&
                   fault.type != FaultType::NONE;
        }
    );

    if (foundFault != faults.end())
    {
        return foundFault->type;
    }

    return FaultType::NONE;
}

void VehicleSimulator::addSignalReading(
    std::vector<SignalReading>& readings,
    SignalId id,
    double normalValue,
    double faultValue) const
{
    FaultType faultType = getFaultType(id);

    if (faultType == FaultType::MISSING)
    {
        return;
    }

    double exposedValue = normalValue;

    if (faultType == FaultType::OUT_OF_RANGE)
    {
        exposedValue = faultValue;
    }

    readings.push_back({id, exposedValue});
}

std::vector<SignalReading> VehicleSimulator::exposeSignals() const
{
    // Build the readings available during this cycle.
    std::vector<SignalReading> readings;
    readings.reserve(6);

    addSignalReading(readings, SignalId::THROTTLE, throttle, 150.0);
    addSignalReading(readings, SignalId::SPEED, speed, 300.0);
    addSignalReading(readings, SignalId::RPM, rpm, 9500.0);
    addSignalReading(readings, SignalId::TEMPERATURE, temperature, 200.0);
    addSignalReading(readings, SignalId::BATTERY_VOLTAGE, batteryVoltage, 20.0);
    addSignalReading(readings, SignalId::OIL_PRESSURE, oilPressure, 15.0);

    return readings;
}

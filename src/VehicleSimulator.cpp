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
    // Update Normal Behavior
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

    // Update Faults
    updateFaults();
}

void VehicleSimulator::updateFaults()
{
    // Update existing faults
    for (int i = 0; i < faults.size(); i++)
    {
        if (faults[i].remainingCycles > 0)
        {
            faults[i].remainingCycles--;
        }

        // If the fault has ended
        if (faults[i].remainingCycles == 0)
        {
            faults[i].type = FaultType::NONE;
        }
    }

    // If a new failure occurs.
    // Probability of a new failure
    std::uniform_real_distribution<double> faultChance(0.0, 1.0);

    double chance = faultChance(generator);

    if (chance < 0.05)
    {
        // Select signal
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

        // Check if the signal already has an active fault
        if (hasActiveFault(selectedId))
        {
            return;
        }

        // Select fault type
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


        // Select fault duration
        std::uniform_int_distribution<int> faultDuration(3, 10);

        int duration = faultDuration(generator);

        // Check if the signal already exists
        bool faultUpdated = false;

        for (std::size_t i = 0; i < faults.size(); i++)
        {
            if (faults[i].id == selectedId &&
                faults[i].type == FaultType::NONE)
            {
                // Reuse the existing record
                faults[i].type = selectedType;
                faults[i].remainingCycles = duration;

                faultUpdated = true;

                break;
            }
        }

        // If the fault doesn't exist, create it
        if (!faultUpdated)
        {
            SignalFault newFault;

            newFault.id = selectedId;
            newFault.type = selectedType;
            newFault.remainingCycles = duration;

            faults.push_back(newFault);
        }
    }
}

bool VehicleSimulator::hasActiveFault(SignalId id) const
{
    for (int i = 0; i < faults.size(); i++)
    {
        if (faults[i].id == id && faults[i].type != FaultType::NONE)
        {
            return true;
        }
    }

    return false;
}

FaultType VehicleSimulator::getFaultType(SignalId id) const
{
    for (int i = 0; i < faults.size(); i++)
    {
        if (faults[i].id == id && faults[i].type != FaultType::NONE)
        {
            return faults[i].type;
        }
    }

    return FaultType::NONE;
}

void VehicleSimulator::addSignal(
    std::vector<SignalVehicle>& signals,
    SignalId id,
    double normalValue,
    double faultValue) const
{
    FaultType fault = getFaultType(id);

    // Do not send the signal
    if (fault == FaultType::MISSING)
    {
        return;
    }

    // Send out-of-range value
    if (fault == FaultType::OUT_OF_RANGE)
    {
        signals.push_back({id, faultValue});
        return;
    }

    // Send normal value
    signals.push_back({id, normalValue});
}

std::vector<SignalVehicle> VehicleSimulator::exposeSignals() const
{
    std::vector<SignalVehicle> signals;

    addSignal(
        signals,
        SignalId::THROTTLE,
        throttle,
        150.0
    );

    addSignal(
        signals,
        SignalId::SPEED,
        speed,
        300.0
    );

    addSignal(
        signals,
        SignalId::RPM,
        rpm,
        8000.0
    );

    addSignal(
        signals,
        SignalId::TEMPERATURE,
        temperature,
        150.0
    );

    addSignal(
        signals,
        SignalId::BATTERY_VOLTAGE,
        batteryVoltage,
        20.0
    );

    addSignal(
        signals,
        SignalId::OIL_PRESSURE,
        oilPressure,
        10.0
    );

    return signals;
}
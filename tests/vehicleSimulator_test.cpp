
#include <gtest/gtest.h>

#include "VehicleSimulator.hpp"

// Verify the initial values of all vehicle signals.
TEST(VehicleSimulatorTest, InitialState)
{
    VehicleSimulator simulator;

    std::vector<SignalReading> readings = simulator.exposeSignals();

    ASSERT_EQ(readings.size(), 6);

    EXPECT_EQ(readings[0].id, SignalId::THROTTLE);
    EXPECT_DOUBLE_EQ(readings[0].value, 0.0);

    EXPECT_EQ(readings[1].id, SignalId::SPEED);
    EXPECT_DOUBLE_EQ(readings[1].value, 0.0);

    EXPECT_EQ(readings[2].id, SignalId::RPM);
    EXPECT_DOUBLE_EQ(readings[2].value, 800.0);

    EXPECT_EQ(readings[3].id, SignalId::TEMPERATURE);
    EXPECT_DOUBLE_EQ(readings[3].value, 25.0);

    EXPECT_EQ(readings[4].id, SignalId::BATTERY_VOLTAGE);
    EXPECT_DOUBLE_EQ(readings[4].value, 12.6);

    EXPECT_EQ(readings[5].id, SignalId::OIL_PRESSURE);
    EXPECT_DOUBLE_EQ(readings[5].value, 1.5);
}


TEST(VehicleSimulatorTest, SignalUpdate)
{
    VehicleSimulator simulator;

    std::vector<SignalReading> initialReadings = simulator.exposeSignals();

    simulator.updateSignal();

    std::vector<SignalReading> updatedReadings = simulator.exposeSignals();

    ASSERT_EQ(initialReadings.size(), 6);
    ASSERT_EQ(updatedReadings.size(), 6);

    bool signalChanged = false;

    for (std::size_t i = 0; i < initialReadings.size(); i++)
    {
        if (initialReadings[i].value != updatedReadings[i].value)
        {
            signalChanged = true;
        }
    }

    EXPECT_TRUE(signalChanged);
}


TEST(VehicleSimulatorTest, SignalLimits)
{
    VehicleSimulator simulator;

    for (int cycle = 0; cycle < 100; cycle++)
    {
        simulator.updateSignal();

        std::vector<SignalReading> readings = simulator.exposeSignals();

        for (const SignalReading& reading : readings)
        {
            if (reading.id == SignalId::THROTTLE)
            {
                EXPECT_GE(reading.value, 0.0);
            }
            else if (reading.id == SignalId::SPEED)
            {
                EXPECT_GE(reading.value, 0.0);
            }
            else if (reading.id == SignalId::RPM)
            {
                EXPECT_GE(reading.value, 700.0);
            }
        }
    }
}


TEST(VehicleSimulatorTest, MissingSignals)
{
    VehicleSimulator simulator;

    bool missingDetected = false;

    for (int cycle = 0; cycle < 1000; cycle++)
    {
        simulator.updateSignal();

        std::vector<SignalReading> readings = simulator.exposeSignals();

        if (readings.size() < 6)
        {
            missingDetected = true;
            break;
        }
    }

    EXPECT_TRUE(missingDetected);
}


TEST(VehicleSimulatorTest, OutOfRangeSignals)
{
    VehicleSimulator simulator;

    bool outOfRangeDetected = false;

    for (int cycle = 0; cycle < 1000; cycle++)
    {
        simulator.updateSignal();

        std::vector<SignalReading> readings = simulator.exposeSignals();

        for (const SignalReading& reading : readings)
        {
            if ((reading.id == SignalId::THROTTLE && reading.value == 150.0) ||
                (reading.id == SignalId::SPEED && reading.value == 300.0) ||
                (reading.id == SignalId::RPM && reading.value == 9500.0) ||
                (reading.id == SignalId::TEMPERATURE && reading.value == 200.0) ||
                (reading.id == SignalId::BATTERY_VOLTAGE && reading.value == 20.0) ||
                (reading.id == SignalId::OIL_PRESSURE && reading.value == 15.0))
            {
                outOfRangeDetected = true;
            }
        }

        if (outOfRangeDetected)
        {
            break;
        }
    }

    EXPECT_TRUE(outOfRangeDetected);
}


TEST(VehicleSimulatorTest, FaultRecovery)
{
    VehicleSimulator simulator;

    bool faultDetected = false;
    bool recovered = false;

    for (int cycle = 0; cycle < 10000; cycle++)
    {
        simulator.updateSignal();

        std::vector<SignalReading> readings = simulator.exposeSignals();

        for (const SignalReading& reading : readings)
        {
            if (reading.id == SignalId::THROTTLE)
            {
                if (reading.value == 150.0)
                {
                    faultDetected = true;
                }

                if (faultDetected &&
                    reading.value >= 0.0 &&
                    reading.value <= 100.0)
                {
                    recovered = true;
                }
            }
        }

        if (recovered)
        {
            break;
        }
    }

    EXPECT_TRUE(faultDetected);
    EXPECT_TRUE(recovered);
}

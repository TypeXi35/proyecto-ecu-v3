#include <gtest/gtest.h>
#include <cstddef>
#include <limits>

#include "GatewayECU.hpp"
#include "SignalLimits.hpp"

// Verify the initial state of all sensors.
TEST(GatewayTest, InitialState)
{
    GatewayECU gateway;

    std::vector<SignalId> signalIds =
    {
        SignalId::SPEED,
        SignalId::RPM,
        SignalId::TEMPERATURE,
        SignalId::THROTTLE,
        SignalId::BATTERY_VOLTAGE,
        SignalId::OIL_PRESSURE
    };

    for (SignalId id : signalIds)
    {
        const Sensor& sensor = gateway.findSensor(id);

        EXPECT_EQ(sensor.getState(), SignalState::NOT_AVAILABLE);
        EXPECT_DOUBLE_EQ(sensor.getValue(), 0.0);
        EXPECT_EQ(sensor.getMissedCycles(), 0u);
    }

    EXPECT_EQ(gateway.getSensors().size(), 6u);
    EXPECT_EQ(gateway.countInvalidSignals(), 6u);
}

// Verify that all valid signals are processed correctly.
TEST(GatewayTest, ValidSignals)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, 60.0},
        {SignalId::RPM, 3000.0},
        {SignalId::TEMPERATURE, 85.0},
        {SignalId::THROTTLE, 40.0},
        {SignalId::BATTERY_VOLTAGE, 13.8},
        {SignalId::OIL_PRESSURE, 3.0}
    };

    gateway.processCycle(readings);

    for (const SignalReading& reading : readings)
    {
        const Sensor& sensor = gateway.findSensor(reading.id);

        EXPECT_EQ(sensor.getState(), SignalState::VALID);
        EXPECT_NEAR(sensor.getValue(), reading.value, 0.000001);
        EXPECT_EQ(sensor.getMissedCycles(), 0u);
    }

    EXPECT_EQ(gateway.countInvalidSignals(), 0u);
}

// Verify that all out-of-range signals are detected.
TEST(GatewayTest, OutOfRangeSignals)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, 300.0},
        {SignalId::RPM, 9500.0},
        {SignalId::TEMPERATURE, 200.0},
        {SignalId::THROTTLE, 150.0},
        {SignalId::BATTERY_VOLTAGE, 20.0},
        {SignalId::OIL_PRESSURE, 15.0}
    };

    gateway.processCycle(readings);

    for (const SignalReading& reading : readings)
    {
        const Sensor& sensor = gateway.findSensor(reading.id);

        EXPECT_EQ(sensor.getState(), SignalState::OUT_OF_RANGE);
        EXPECT_NEAR(sensor.getValue(), reading.value, 0.000001);
        EXPECT_EQ(sensor.getMissedCycles(), 0u);
    }

    EXPECT_EQ(gateway.countInvalidSignals(), 6u);
}

// Verify that all operating limits are accepted as valid.
TEST(GatewayTest, OperatingLimits)
{
    GatewayECU gateway;

    std::vector<SignalReading> minimumReadings =
    {
        {SignalId::SPEED, SPEED_MIN_KMH},
        {SignalId::RPM, RPM_MIN},
        {SignalId::TEMPERATURE, TEMP_MIN_C},
        {SignalId::THROTTLE, THROTTLE_MIN_PCT},
        {SignalId::BATTERY_VOLTAGE, VOLTAGE_MIN_V},
        {SignalId::OIL_PRESSURE, OIL_PRESSURE_MIN_BAR}
    };

    gateway.processCycle(minimumReadings);

    for (const SignalReading& reading : minimumReadings)
    {
        const Sensor& sensor = gateway.findSensor(reading.id);

        EXPECT_EQ(sensor.getState(), SignalState::VALID);
        EXPECT_NEAR(sensor.getValue(), reading.value, 0.000001);
        EXPECT_EQ(sensor.getMissedCycles(), 0u);
    }

    EXPECT_EQ(gateway.countInvalidSignals(), 0u);

    std::vector<SignalReading> maximumReadings =
    {
        {SignalId::SPEED, SPEED_MAX_KMH},
        {SignalId::RPM, RPM_MAX},
        {SignalId::TEMPERATURE, TEMP_MAX_C},
        {SignalId::THROTTLE, THROTTLE_MAX_PCT},
        {SignalId::BATTERY_VOLTAGE, VOLTAGE_MAX_V},
        {SignalId::OIL_PRESSURE, OIL_PRESSURE_MAX_BAR}
    };

    gateway.processCycle(maximumReadings);

    for (const SignalReading& reading : maximumReadings)
    {
        const Sensor& sensor = gateway.findSensor(reading.id);

        EXPECT_EQ(sensor.getState(), SignalState::VALID);
        EXPECT_NEAR(sensor.getValue(), reading.value, 0.000001);
        EXPECT_EQ(sensor.getMissedCycles(), 0u);
    }

    EXPECT_EQ(gateway.countInvalidSignals(), 0u);
}


// Verify physical values after encoding and decoding.
TEST(GatewayTest, ScalingPrecision)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, 60.04},
        {SignalId::RPM, 2500.4},
        {SignalId::TEMPERATURE, 85.04},
        {SignalId::THROTTLE, 40.04},
        {SignalId::BATTERY_VOLTAGE, 13.804},
        {SignalId::OIL_PRESSURE, 3.02}
    };

    std::vector<SignalReading> expectedReadings =
    {
        {SignalId::SPEED, 60.0},
        {SignalId::RPM, 2500.0},
        {SignalId::TEMPERATURE, 85.0},
        {SignalId::THROTTLE, 40.0},
        {SignalId::BATTERY_VOLTAGE, 13.80},
        {SignalId::OIL_PRESSURE, 3.0}
    };

    gateway.processCycle(readings);

    for (const SignalReading& expected : expectedReadings)
    {
        const Sensor& sensor = gateway.findSensor(expected.id);

        EXPECT_EQ(sensor.getState(), SignalState::VALID);
        EXPECT_NEAR(sensor.getValue(), expected.value, 0.000001);
        EXPECT_EQ(sensor.getMissedCycles(), 0u);
    }

    EXPECT_EQ(gateway.countInvalidSignals(), 0u);
}


// Verify that missing signals are detected after the allowed cycles.
TEST(GatewayTest, MissingSignals)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, 60.0},
        {SignalId::RPM, 3000.0},
        {SignalId::TEMPERATURE, 85.0},
        {SignalId::THROTTLE, 40.0},
        {SignalId::BATTERY_VOLTAGE, 13.8},
        {SignalId::OIL_PRESSURE, 3.0}
    };

    gateway.processCycle(readings);

    readings.erase(readings.begin() + 2);

    for (unsigned int cycle = 1; cycle <= MAX_MISSED_CYCLES; ++cycle)
    {
        gateway.processCycle(readings);

        const Sensor& temperature = gateway.findSensor(SignalId::TEMPERATURE);

        EXPECT_EQ(temperature.getState(), SignalState::VALID);
        EXPECT_EQ(temperature.getMissedCycles(), cycle);
        EXPECT_DOUBLE_EQ(temperature.getValue(), 85.0);
    }

    gateway.processCycle(readings);

    const Sensor& temperature = gateway.findSensor(SignalId::TEMPERATURE);

    EXPECT_EQ(temperature.getState(), SignalState::NOT_AVAILABLE);
    EXPECT_EQ(temperature.getMissedCycles(), MAX_MISSED_CYCLES + 1);
    EXPECT_DOUBLE_EQ(temperature.getValue(), 85.0);

    for (const SignalReading& reading : readings)
    {
        const Sensor& sensor = gateway.findSensor(reading.id);

        EXPECT_EQ(sensor.getState(), SignalState::VALID);
        EXPECT_EQ(sensor.getMissedCycles(), 0u);
    }

    EXPECT_EQ(gateway.countInvalidSignals(), 1u);
}


// Verify that a missing signal recovers when a new reading arrives.
TEST(GatewayTest, SignalRecovery)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::TEMPERATURE, 85.0}
    };

    gateway.processCycle(readings);

    const Sensor& temperature = gateway.findSensor(SignalId::TEMPERATURE);

    EXPECT_EQ(temperature.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(temperature.getValue(), 85.0);
    EXPECT_EQ(temperature.getMissedCycles(), 0u);

    readings.clear();

    for (unsigned int cycle = 0; cycle <= MAX_MISSED_CYCLES; ++cycle)
    {
        gateway.processCycle(readings);
    }

    EXPECT_EQ(temperature.getState(), SignalState::NOT_AVAILABLE);
    EXPECT_EQ(temperature.getMissedCycles(), MAX_MISSED_CYCLES + 1);
    EXPECT_DOUBLE_EQ(temperature.getValue(), 85.0);

    readings.push_back({SignalId::TEMPERATURE, 90.0});

    gateway.processCycle(readings);

    EXPECT_EQ(temperature.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(temperature.getValue(), 90.0);
    EXPECT_EQ(temperature.getMissedCycles(), 0u);
}


// Verify that an out-of-range signal recovers with a valid reading.
TEST(GatewayTest, OutOfRangeRecovery)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, 60.0}
    };

    gateway.processCycle(readings);

    const Sensor& speed = gateway.findSensor(SignalId::SPEED);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), 0u);

    readings =
    {
        {SignalId::SPEED, 300.0}
    };

    gateway.processCycle(readings);

    EXPECT_EQ(speed.getState(), SignalState::OUT_OF_RANGE);
    EXPECT_DOUBLE_EQ(speed.getValue(), 300.0);
    EXPECT_EQ(speed.getMissedCycles(), 0u);

    readings =
    {
        {SignalId::SPEED, 80.0}
    };

    gateway.processCycle(readings);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 80.0);
    EXPECT_EQ(speed.getMissedCycles(), 0u);
}


// Verify that searching for an invalid sensor ID throws an exception.
TEST(GatewayTest, InvalidSignalId)
{
    GatewayECU gateway;

    SignalId invalidId = static_cast<SignalId>(255);

    EXPECT_THROW(
        gateway.findSensor(invalidId),
        std::out_of_range
    );

    EXPECT_EQ(gateway.getSensors().size(), 6u);
    EXPECT_EQ(gateway.countInvalidSignals(), 6u);
}


// Verify that non-finite physical values are rejected.
TEST(GatewayTest, InvalidPhysicalValues)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, 60.0}
    };

    gateway.processCycle(readings);

    const Sensor& speed = gateway.findSensor(SignalId::SPEED);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), 0u);

    std::vector<double> invalidValues =
    {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()
    };

    for (std::size_t i = 0; i < invalidValues.size(); ++i)
    {
        readings =
        {
            {SignalId::SPEED, invalidValues[i]}
        };

        gateway.processCycle(readings);

        EXPECT_EQ(speed.getState(), SignalState::VALID);
        EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
        EXPECT_EQ(speed.getMissedCycles(), i + 1);
    }

    readings =
    {
        {SignalId::SPEED, std::numeric_limits<double>::quiet_NaN()}
    };

    gateway.processCycle(readings);

    EXPECT_EQ(speed.getState(), SignalState::NOT_AVAILABLE);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), MAX_MISSED_CYCLES + 1);
}


// Verify that values outside the raw encoding range are rejected.
TEST(GatewayTest, UnrepresentableValues)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, 60.0}
    };

    gateway.processCycle(readings);

    const Sensor& speed = gateway.findSensor(SignalId::SPEED);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), 0u);

    readings =
    {
        {SignalId::SPEED, -10.0}
    };

    gateway.processCycle(readings);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), 1u);

    readings =
    {
        {SignalId::SPEED, 7000.0}
    };

    gateway.processCycle(readings);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), 2u);
}


// Verify recovery after an unrepresentable physical reading.
TEST(GatewayTest, InvalidReadingRecovery)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, 60.0}
    };

    gateway.processCycle(readings);

    const Sensor& speed = gateway.findSensor(SignalId::SPEED);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), 0u);

    readings =
    {
        {SignalId::SPEED, -10.0}
    };

    gateway.processCycle(readings);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), 1u);

    readings =
    {
        {SignalId::SPEED, 80.0}
    };

    gateway.processCycle(readings);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 80.0);
    EXPECT_EQ(speed.getMissedCycles(), 0u);
}


// Verify that an invalid reading does not affect other sensors.
TEST(GatewayTest, IndependentSensorProcessing)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, -10.0},
        {SignalId::RPM, 3000.0},
        {SignalId::TEMPERATURE, 85.0},
        {SignalId::THROTTLE, 40.0},
        {SignalId::BATTERY_VOLTAGE, 13.8},
        {SignalId::OIL_PRESSURE, 3.0}
    };

    gateway.processCycle(readings);

    const Sensor& speed = gateway.findSensor(SignalId::SPEED);

    EXPECT_EQ(speed.getState(), SignalState::NOT_AVAILABLE);
    EXPECT_DOUBLE_EQ(speed.getValue(), 0.0);
    EXPECT_EQ(speed.getMissedCycles(), 1u);

    for (const SignalReading& reading : readings)
    {
        if (reading.id == SignalId::SPEED)
        {
            continue;
        }

        const Sensor& sensor = gateway.findSensor(reading.id);

        EXPECT_EQ(sensor.getState(), SignalState::VALID);
        EXPECT_NEAR(sensor.getValue(), reading.value, 0.000001);
        EXPECT_EQ(sensor.getMissedCycles(), 0u);
    }

    EXPECT_EQ(gateway.countInvalidSignals(), 1u);
}


// Verify sensor behavior after repeated encoding failures.
TEST(GatewayTest, RepeatedInvalidReadings)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, 60.0}
    };

    gateway.processCycle(readings);

    const Sensor& speed = gateway.findSensor(SignalId::SPEED);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), 0u);

    readings =
    {
        {SignalId::SPEED, -10.0}
    };

    for (unsigned int cycle = 1; cycle <= MAX_MISSED_CYCLES; ++cycle)
    {
        gateway.processCycle(readings);

        EXPECT_EQ(speed.getState(), SignalState::VALID);
        EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
        EXPECT_EQ(speed.getMissedCycles(), cycle);
    }

    gateway.processCycle(readings);

    EXPECT_EQ(speed.getState(), SignalState::NOT_AVAILABLE);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), MAX_MISSED_CYCLES + 1u);

    readings =
    {
        {SignalId::SPEED, 80.0}
    };

    gateway.processCycle(readings);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 80.0);
    EXPECT_EQ(speed.getMissedCycles(), 0u);
}


// Verify that only the first reading is used for duplicate signal IDs.
TEST(GatewayTest, DuplicateSignalIds)
{
    GatewayECU gateway;

    std::vector<SignalReading> readings =
    {
        {SignalId::SPEED, 60.0},
        {SignalId::SPEED, 120.0},
        {SignalId::RPM, 3000.0}
    };

    gateway.processCycle(readings);

    const Sensor& speed = gateway.findSensor(SignalId::SPEED);

    EXPECT_EQ(speed.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(speed.getValue(), 60.0);
    EXPECT_EQ(speed.getMissedCycles(), 0u);

    const Sensor& rpm = gateway.findSensor(SignalId::RPM);

    EXPECT_EQ(rpm.getState(), SignalState::VALID);
    EXPECT_DOUBLE_EQ(rpm.getValue(), 3000.0);
    EXPECT_EQ(rpm.getMissedCycles(), 0u);

    EXPECT_EQ(gateway.countInvalidSignals(), 4u);
}

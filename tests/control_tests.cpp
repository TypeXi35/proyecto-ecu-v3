#include <gtest/gtest.h>

#include <vector>

#include "ControlECU.hpp"
#include "SignalLimits.hpp"

namespace
{
    // Valid values inside every operating range
    constexpr double HEALTHY_SPEED       = 60.0;
    constexpr double HEALTHY_THROTTLE    = 35.0;
    constexpr double HEALTHY_TEMPERATURE = 90.0;
    constexpr double HEALTHY_BATTERY     = 13.8;
    constexpr double HEALTHY_OIL         = 2.7;
    // Coherent with speed and throttle: rpm = 800 + 22 * speed + 18 * throttle
    constexpr double HEALTHY_RPM = 800.0 + 22.0 * HEALTHY_SPEED + 18.0 * HEALTHY_THROTTLE;
    
    constexpr double OIL_ABOVE_MAX = 6.1;
    constexpr double TEMPERATURE_ABOVE_MAX = 110.1;
    constexpr double OIL_BELOW_MIN = 0.7;
    constexpr double TEMPERATURE_BELOW_MIN = -30.1;

    // Just outside the operating range of each non-critical signal
    constexpr double SPEED_BELOW_MIN    = -0.1;    // min 0 km/h
    constexpr double SPEED_ABOVE_MAX    = 200.1;   // max 200 km/h
    constexpr double RPM_BELOW_MIN      = -0.1;    // min 0 rpm
    constexpr double RPM_ABOVE_MAX      = 6800.1;  // max 6800 rpm
    constexpr double THROTTLE_BELOW_MIN = -0.1;    // min 0 %
    constexpr double THROTTLE_ABOVE_MAX = 100.1;   // max 100 %
    constexpr double BATTERY_BELOW_MIN  = 11.4;    // min 11.5 V
    constexpr double BATTERY_ABOVE_MAX  = 15.1;    // max 15 V

    Sensor receivedSensor(SignalId id, double min, double max, double value)
    {
        Sensor sensor(id, "", min, max, MAX_MISSED_CYCLES);
        sensor.update(value);
        return sensor;
    }

    std::vector<Sensor> healthySensors() {
    return {
        receivedSensor(SignalId::SPEED,           SPEED_MIN_KMH,        SPEED_MAX_KMH,        HEALTHY_SPEED),
        receivedSensor(SignalId::RPM,             RPM_MIN,              RPM_MAX,              HEALTHY_RPM),
        receivedSensor(SignalId::TEMPERATURE,     TEMP_MIN_C,           TEMP_MAX_C,           HEALTHY_TEMPERATURE),
        receivedSensor(SignalId::THROTTLE,        THROTTLE_MIN_PCT,     THROTTLE_MAX_PCT,     HEALTHY_THROTTLE),
        receivedSensor(SignalId::BATTERY_VOLTAGE, VOLTAGE_MIN_V,        VOLTAGE_MAX_V,        HEALTHY_BATTERY),
        receivedSensor(SignalId::OIL_PRESSURE,    OIL_PRESSURE_MIN_BAR, OIL_PRESSURE_MAX_BAR, HEALTHY_OIL),
    };
    }
}

TEST(ControlECUTest, StartsInInit)
{
    const ControlECU control;
    EXPECT_EQ(control.getControlData().currentState, ECUState::INIT);
}

TEST(ControlECUTest, StartsCycleAt0)
{
    const ControlECU control;
    EXPECT_EQ(control.getControlData().cycleCount, 0u);
}

TEST(ControlECUTest, StartsWithEmptySensors)
{
    const ControlECU control;
    const ECUData &data = control.getControlData();
    EXPECT_EQ(data.sensors.batteryVoltage.getState(), SignalState::NOT_AVAILABLE);
    EXPECT_EQ(data.sensors.oilPressure.getState(), SignalState::NOT_AVAILABLE);
    EXPECT_EQ(data.sensors.rpm.getState(), SignalState::NOT_AVAILABLE);
    EXPECT_EQ(data.sensors.speed.getState(), SignalState::NOT_AVAILABLE);
    EXPECT_EQ(data.sensors.temperature.getState(), SignalState::NOT_AVAILABLE);
    EXPECT_EQ(data.sensors.throttle.getState(), SignalState::NOT_AVAILABLE);
}

TEST(ControlECUTest, HealthyCycleUpdate)
{
    ControlECU control;
    const ECUData &data = control.getControlData();
    control.runControlCycle(healthySensors());
    EXPECT_EQ(data.sensors.batteryVoltage.getValue(), HEALTHY_BATTERY);
    EXPECT_EQ(data.sensors.oilPressure.getValue(), HEALTHY_OIL);
    EXPECT_EQ(data.sensors.rpm.getValue(), HEALTHY_RPM);
    EXPECT_EQ(data.sensors.speed.getValue(), HEALTHY_SPEED);
    EXPECT_EQ(data.sensors.temperature.getValue(), HEALTHY_TEMPERATURE);
    EXPECT_EQ(data.sensors.throttle.getValue(), HEALTHY_THROTTLE);
}

#include <gtest/gtest.h>

#include <array>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include "CSVLogger.hpp"

namespace
{
    // Expected CSV schema, including the order of all sensor fields.
    const std::string expectedHeader =
        "cycleCount,state,faultCode,"
        "speed_value,speed_min,speed_max,speed_missedCycles,speed_maxMissedCycles,speed_state,"
        "rpm_value,rpm_min,rpm_max,rpm_missedCycles,rpm_maxMissedCycles,rpm_state,"
        "temperature_value,temperature_min,temperature_max,temperature_missedCycles,temperature_maxMissedCycles,temperature_state,"
        "batteryVoltage_value,batteryVoltage_min,batteryVoltage_max,batteryVoltage_missedCycles,batteryVoltage_maxMissedCycles,batteryVoltage_state,"
        "throttle_value,throttle_min,throttle_max,throttle_missedCycles,throttle_maxMissedCycles,throttle_state,"
        "oilPressure_value,oilPressure_min,oilPressure_max,oilPressure_missedCycles,oilPressure_maxMissedCycles,oilPressure_state";

    // Create a reusable baseline with valid ECU and sensor data.
    ECUData createValidECUData()
    {
        ECUData ecu{};

        ecu.cycleCount = 1;
        ecu.currentState = ECUState::OPERATIONAL;
        ecu.faultCode = FaultCode::NONE;

        ecu.sensors.speed =
            Sensor(SignalId::SPEED, "km/h", 0.0, 240.0, 3);

        ecu.sensors.rpm =
            Sensor(SignalId::RPM, "rpm", 0.0, 8000.0, 3);

        ecu.sensors.temperature =
            Sensor(SignalId::TEMPERATURE, "C", -40.0, 150.0, 3);

        ecu.sensors.batteryVoltage =
            Sensor(SignalId::BATTERY_VOLTAGE, "V", 0.0, 16.0, 3);

        ecu.sensors.throttle =
            Sensor(SignalId::THROTTLE, "%", 0.0, 100.0, 3);

        ecu.sensors.oilPressure =
            Sensor(SignalId::OIL_PRESSURE, "bar", 0.0, 10.0, 3);

        ecu.sensors.speed.update(80.0);
        ecu.sensors.rpm.update(2500.0);
        ecu.sensors.temperature.update(90.0);
        ecu.sensors.batteryVoltage.update(12.6);
        ecu.sensors.throttle.update(40.0);
        ecu.sensors.oilPressure.update(3.0);

        return ecu;
    }

    class CSVLoggerTest : public ::testing::Test
    {
        protected:

            std::filesystem::path testDirectory;
            std::filesystem::path testFile;

            bool directoryCreated{false};

            void SetUp() override
            {
                // Isolate each test in its own temporary directory.
                std::random_device random;

                testDirectory =
                    std::filesystem::temp_directory_path() /
                    ("csv_logger_test_" + std::to_string(random()));

                directoryCreated =
                    std::filesystem::create_directory(testDirectory);

                ASSERT_TRUE(directoryCreated);

                testFile = testDirectory / "test_log.csv";
            }

            void TearDown() override
            {
                // Only remove a directory successfully created by this fixture.
                if (!directoryCreated)
                {
                    return;
                }

                std::error_code error;

                std::filesystem::remove_all(testDirectory, error);

                EXPECT_FALSE(error);
            }
    };
}

namespace ConstructorTests
{
    // Verify that a new CSV file contains only the expected header.
    TEST_F(CSVLoggerTest, CreatesFileWithHeader)
    {
        ASSERT_FALSE(std::filesystem::exists(testFile));

        CSVLogger logger(testFile);

        ASSERT_TRUE(std::filesystem::exists(testFile));

        std::ifstream file(testFile);

        ASSERT_TRUE(file.is_open());

        std::string header;

        ASSERT_TRUE(std::getline(file, header));

        EXPECT_EQ(header, expectedHeader);

        std::string entry;

        EXPECT_FALSE(std::getline(file, entry));
    }

    // Verify that an existing empty file is initialized with the expected header.
    TEST_F(CSVLoggerTest, CreatesHeaderInEmptyFile)
    {
        std::ofstream emptyFile(testFile);

        ASSERT_TRUE(emptyFile.is_open());

        emptyFile.close();

        ASSERT_TRUE(std::filesystem::exists(testFile));
        ASSERT_EQ(std::filesystem::file_size(testFile), 0u);

        CSVLogger logger(testFile);

        std::ifstream file(testFile);

        ASSERT_TRUE(file.is_open());

        std::string header;

        ASSERT_TRUE(std::getline(file, header));

        EXPECT_EQ(header, expectedHeader);

        std::string entry;

        EXPECT_FALSE(std::getline(file, entry));
    }

    // Verify that initialization preserves the existing header and records.
    TEST_F(CSVLoggerTest, PreservesExistingFile)
    {
        const std::string existingEntry =
            "1,OPERATIONAL,NONE,"
            "80,0,240,0,3,VALID,"
            "2500,0,8000,0,3,VALID,"
            "90,-40,150,0,3,VALID,"
            "12.6,0,16,0,3,VALID,"
            "40,0,100,0,3,VALID,"
            "3,0,10,0,3,VALID";

        std::ofstream existingFile(testFile);

        ASSERT_TRUE(existingFile.is_open());

        existingFile << expectedHeader << '\n';
        existingFile << existingEntry << '\n';

        existingFile.close();

        ASSERT_TRUE(std::filesystem::exists(testFile));
        ASSERT_GT(std::filesystem::file_size(testFile), 0u);

        CSVLogger logger(testFile);

        std::ifstream file(testFile);

        ASSERT_TRUE(file.is_open());

        std::string header;

        ASSERT_TRUE(std::getline(file, header));

        EXPECT_EQ(header, expectedHeader);

        std::string entry;

        ASSERT_TRUE(std::getline(file, entry));

        EXPECT_EQ(entry, existingEntry);

        EXPECT_FALSE(std::getline(file, entry));
    }
}

namespace EncodeTests
{
    // Verify that valid ECU data is serialized into the expected CSV record.
    TEST_F(CSVLoggerTest, EncodesValidRecord)
    {
        CSVLogger logger(testFile);

        ECUData ecu = createValidECUData();

        logger(ecu);

        const std::string entry = logger.encode();

        const std::string expectedEntry =
            "1,OPERATIONAL,NONE,"
            "80,0,240,0,3,VALID,"
            "2500,0,8000,0,3,VALID,"
            "90,-40,150,0,3,VALID,"
            "12.6,0,16,0,3,VALID,"
            "40,0,100,0,3,VALID,"
            "3,0,10,0,3,VALID";

        EXPECT_EQ(entry, expectedEntry);
    }

    // Verify that out-of-range values and their signal states are preserved during encoding.
    TEST_F(CSVLoggerTest, EncodesOutOfRangeSignals)
    {
        CSVLogger logger(testFile);

        ECUData ecu = createValidECUData();

        ecu.cycleCount = 2;
        ecu.currentState = ECUState::DEGRADED;
        ecu.faultCode = FaultCode::SIGNAL_OUT_OF_LIMITS;

        ecu.sensors.speed.update(300.0);
        ecu.sensors.temperature.update(200.0);
        ecu.sensors.oilPressure.update(-1.0);

        logger(ecu);

        const std::string entry = logger.encode();

        const std::string expectedEntry =
            "2,DEGRADED,SIGNAL_OUT_OF_LIMITS,"
            "300,0,240,0,3,OUT_OF_RANGE,"
            "2500,0,8000,0,3,VALID,"
            "200,-40,150,0,3,OUT_OF_RANGE,"
            "12.6,0,16,0,3,VALID,"
            "40,0,100,0,3,VALID,"
            "-1,0,10,0,3,OUT_OF_RANGE";

        EXPECT_EQ(entry, expectedEntry);
    }

    // Verify that encoding preserves missed-cycle counters and signal availability states.
    TEST_F(CSVLoggerTest, EncodesMissingSignals)
    {
        CSVLogger logger(testFile);

        ECUData ecu = createValidECUData();

        ecu.cycleCount = 6;
        ecu.currentState = ECUState::SAFE_STATE;
        ecu.faultCode = FaultCode::CRITICAL_SENSOR_LOST;

        // Exceed the missed-cycle limit for speed and temperature.
        for (unsigned int i = 0; i < 4; ++i)
        {
            ecu.sensors.speed.registerMissedCycle();
        }

        for (unsigned int i = 0; i < 5; ++i)
        {
            ecu.sensors.temperature.registerMissedCycle();
        }

        // Keep battery voltage and throttle at or below the missed-cycle limit.
        for (unsigned int i = 0; i < 2; ++i)
        {
            ecu.sensors.batteryVoltage.registerMissedCycle();
        }

        for (unsigned int i = 0; i < 3; ++i)
        {
            ecu.sensors.throttle.registerMissedCycle();
        }

        logger(ecu);

        const std::string entry = logger.encode();

        const std::string expectedEntry =
            "6,SAFE_STATE,CRITICAL_SENSOR_LOST,"
            "80,0,240,4,3,NOT_AVAILABLE,"
            "2500,0,8000,0,3,VALID,"
            "90,-40,150,5,3,NOT_AVAILABLE,"
            "12.6,0,16,2,3,VALID,"
            "40,0,100,3,3,VALID,"
            "3,0,10,0,3,VALID";

        EXPECT_EQ(entry, expectedEntry);
    }

    // Verify that each supported ECU state is encoded using its expected text representation.
    TEST_F(CSVLoggerTest, EncodesECUStates)
    {
        CSVLogger logger(testFile);

        ECUData ecu = createValidECUData();

        const std::array<std::pair<ECUState, std::string>, 6> testCases{{
            {ECUState::INIT, "INIT"},
            {ECUState::SELF_TEST, "SELF_TEST"},
            {ECUState::OPERATIONAL, "OPERATIONAL"},
            {ECUState::DEGRADED, "DEGRADED"},
            {ECUState::SAFE_STATE, "SAFE_STATE"},
            {ECUState::SHUTDOWN, "SHUTDOWN"}
        }};

        for (const auto& [state, expectedState] : testCases)
        {
            ecu.currentState = state;

            logger(ecu);

            const std::string entry = logger.encode();

            const std::string expectedEntry =
                "1," + expectedState + ",NONE,"
                "80,0,240,0,3,VALID,"
                "2500,0,8000,0,3,VALID,"
                "90,-40,150,0,3,VALID,"
                "12.6,0,16,0,3,VALID,"
                "40,0,100,0,3,VALID,"
                "3,0,10,0,3,VALID";

            EXPECT_EQ(entry, expectedEntry)
                << "ECU state: " << expectedState;
        }
    }

    // Verify that each supported fault code is encoded using its expected text representation.
    TEST_F(CSVLoggerTest, EncodesFaultCodes)
    {
        CSVLogger logger(testFile);

        ECUData ecu = createValidECUData();

        const std::array<std::pair<FaultCode, std::string>, 6> testCases{{
            {FaultCode::NONE, "NONE"},
            {FaultCode::CRITICAL_SENSOR_LOST, "CRITICAL_SENSOR_LOST"},
            {FaultCode::CRITICAL_OUT_OF_LIMITS, "CRITICAL_OUT_OF_LIMITS"},
            {FaultCode::SENSOR_DEGRADED, "SENSOR_DEGRADED"},
            {FaultCode::SIGNAL_OUT_OF_LIMITS, "SIGNAL_OUT_OF_LIMITS"},
            {FaultCode::INCOHERENT_SIGNALS, "INCOHERENT_SIGNALS"}
        }};

        for (const auto& [faultCode, expectedFaultCode] : testCases)
        {
            ecu.faultCode = faultCode;

            logger(ecu);

            const std::string entry = logger.encode();

            const std::string expectedEntry =
                "1,OPERATIONAL," + expectedFaultCode + ","
                "80,0,240,0,3,VALID,"
                "2500,0,8000,0,3,VALID,"
                "90,-40,150,0,3,VALID,"
                "12.6,0,16,0,3,VALID,"
                "40,0,100,0,3,VALID,"
                "3,0,10,0,3,VALID";

            EXPECT_EQ(entry, expectedEntry)
                << "Fault code: " << expectedFaultCode;
        }
    }

    // Verify that unsupported ECU states and fault codes are encoded as UNKNOWN.
    TEST_F(CSVLoggerTest, EncodesUnknownEnums)
    {
        CSVLogger logger(testFile);

        ECUData ecu = createValidECUData();

        const ECUState unknownState = static_cast<ECUState>(99);
        const FaultCode unknownFaultCode = static_cast<FaultCode>(99);

        const std::string expectedSensors =
            "80,0,240,0,3,VALID,"
            "2500,0,8000,0,3,VALID,"
            "90,-40,150,0,3,VALID,"
            "12.6,0,16,0,3,VALID,"
            "40,0,100,0,3,VALID,"
            "3,0,10,0,3,VALID";

        // Scenario 1: Unknown ECU state only.
        ecu.currentState = unknownState;
        ecu.faultCode = FaultCode::NONE;

        logger(ecu);

        EXPECT_EQ(
            logger.encode(),
            "1,UNKNOWN,NONE," + expectedSensors
        ) << "Unknown ECU state";

        // Scenario 2: Unknown fault code only.
        ecu.currentState = ECUState::OPERATIONAL;
        ecu.faultCode = unknownFaultCode;

        logger(ecu);

        EXPECT_EQ(
            logger.encode(),
            "1,OPERATIONAL,UNKNOWN," + expectedSensors
        ) << "Unknown fault code";

        // Scenario 3: Both enum values are unknown.
        ecu.currentState = unknownState;
        ecu.faultCode = unknownFaultCode;

        logger(ecu);

        EXPECT_EQ(
            logger.encode(),
            "1,UNKNOWN,UNKNOWN," + expectedSensors
        ) << "Unknown ECU state and fault code";
    }

    // Verify that encoding reflects updated ECU and sensor data from a subsequent cycle.
    TEST_F(CSVLoggerTest, EncodesUpdatedRecord)
    {
        CSVLogger logger(testFile);

        ECUData ecu = createValidECUData();

        logger(ecu);

        const std::string firstEntry = logger.encode();

        const std::string expectedFirstEntry =
            "1,OPERATIONAL,NONE,"
            "80,0,240,0,3,VALID,"
            "2500,0,8000,0,3,VALID,"
            "90,-40,150,0,3,VALID,"
            "12.6,0,16,0,3,VALID,"
            "40,0,100,0,3,VALID,"
            "3,0,10,0,3,VALID";

        EXPECT_EQ(firstEntry, expectedFirstEntry);

        // Change ECU data and sensor readings for the next cycle.
        ecu.cycleCount = 2;
        ecu.currentState = ECUState::DEGRADED;
        ecu.faultCode = FaultCode::SIGNAL_OUT_OF_LIMITS;

        ecu.sensors.speed.update(100.0);
        ecu.sensors.rpm.update(3200.0);
        ecu.sensors.temperature.update(200.0);
        ecu.sensors.batteryVoltage.update(13.2);
        ecu.sensors.throttle.update(60.0);
        ecu.sensors.oilPressure.update(4.5);

        // A single missed reading remains within the allowed limit.
        ecu.sensors.batteryVoltage.registerMissedCycle();

        logger(ecu);

        const std::string secondEntry = logger.encode();

        const std::string expectedSecondEntry =
            "2,DEGRADED,SIGNAL_OUT_OF_LIMITS,"
            "100,0,240,0,3,VALID,"
            "3200,0,8000,0,3,VALID,"
            "200,-40,150,0,3,OUT_OF_RANGE,"
            "13.2,0,16,1,3,VALID,"
            "60,0,100,0,3,VALID,"
            "4.5,0,10,0,3,VALID";

        EXPECT_EQ(secondEntry, expectedSecondEntry);
    }

    // Verify that repeated encoding produces consistent output without modifying the CSV file.
    TEST_F(CSVLoggerTest, EncodeDoesNotModifyStorage)
    {
        CSVLogger logger(testFile);

        ECUData ecu = createValidECUData();

        logger(ecu);

        // Capture the original file bytes for the storage comparison.
        std::ifstream fileBefore(testFile, std::ios::binary);

        ASSERT_TRUE(fileBefore.is_open());

        const std::string contentBefore(
            (std::istreambuf_iterator<char>(fileBefore)),
            std::istreambuf_iterator<char>()
        );

        fileBefore.close();

        ASSERT_FALSE(contentBefore.empty());

        const std::string firstEntry = logger.encode();
        const std::string secondEntry = logger.encode();
        const std::string thirdEntry = logger.encode();

        EXPECT_EQ(firstEntry, secondEntry);
        EXPECT_EQ(firstEntry, thirdEntry);

        std::ifstream fileAfter(testFile, std::ios::binary);

        ASSERT_TRUE(fileAfter.is_open());

        const std::string contentAfter(
            (std::istreambuf_iterator<char>(fileAfter)),
            std::istreambuf_iterator<char>()
        );

        EXPECT_EQ(contentBefore, contentAfter);
    }
}

namespace AppendToStorageTests
{
    // Verify that a single entry is appended after the header with a trailing newline.
    TEST_F(CSVLoggerTest, AppendsSingleEntry)
    {
        CSVLogger logger(testFile);

        const std::string entry =
            "1,OPERATIONAL,NONE,"
            "80,0,240,0,3,VALID,"
            "2500,0,8000,0,3,VALID,"
            "90,-40,150,0,3,VALID,"
            "12.6,0,16,0,3,VALID,"
            "40,0,100,0,3,VALID,"
            "3,0,10,0,3,VALID";

        logger.appendToStorage(entry);

        std::ifstream file(testFile);

        ASSERT_TRUE(file.is_open());

        const std::string actualContent(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        const std::string expectedContent =
            expectedHeader + "\n" + entry + "\n";

        EXPECT_EQ(actualContent, expectedContent);
    }

    // Verify that multiple entries are appended in order without overwriting previous records.
    TEST_F(CSVLoggerTest, AppendsMultipleEntries)
    {
        CSVLogger logger(testFile);

        const std::string entry1 =
            "1,OPERATIONAL,NONE,"
            "80,0,240,0,3,VALID,"
            "2500,0,8000,0,3,VALID,"
            "90,-40,150,0,3,VALID,"
            "12.6,0,16,0,3,VALID,"
            "40,0,100,0,3,VALID,"
            "3,0,10,0,3,VALID";

        const std::string entry2 =
            "2,OPERATIONAL,NONE,"
            "100,0,240,0,3,VALID,"
            "3200,0,8000,0,3,VALID,"
            "95,-40,150,0,3,VALID,"
            "13.2,0,16,0,3,VALID,"
            "60,0,100,0,3,VALID,"
            "4.5,0,10,0,3,VALID";

        const std::string entry3 =
            "3,DEGRADED,SIGNAL_OUT_OF_LIMITS,"
            "300,0,240,0,3,OUT_OF_RANGE,"
            "3500,0,8000,0,3,VALID,"
            "100,-40,150,0,3,VALID,"
            "13.5,0,16,0,3,VALID,"
            "70,0,100,0,3,VALID,"
            "5,0,10,0,3,VALID";

        logger.appendToStorage(entry1);
        logger.appendToStorage(entry2);
        logger.appendToStorage(entry3);

        std::ifstream file(testFile);

        ASSERT_TRUE(file.is_open());

        const std::string actualContent(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        const std::string expectedContent =
            expectedHeader + "\n" +
            entry1 + "\n" +
            entry2 + "\n" +
            entry3 + "\n";

        EXPECT_EQ(actualContent, expectedContent);
    }

    // Verify that reopening the logger preserves existing data when appending a new entry.
    TEST_F(CSVLoggerTest, AppendsToExistingFile)
    {
        const std::string entry1 =
            "1,OPERATIONAL,NONE,"
            "80,0,240,0,3,VALID,"
            "2500,0,8000,0,3,VALID,"
            "90,-40,150,0,3,VALID,"
            "12.6,0,16,0,3,VALID,"
            "40,0,100,0,3,VALID,"
            "3,0,10,0,3,VALID";

        const std::string entry2 =
            "2,OPERATIONAL,NONE,"
            "100,0,240,0,3,VALID,"
            "3200,0,8000,0,3,VALID,"
            "95,-40,150,0,3,VALID,"
            "13.2,0,16,0,3,VALID,"
            "60,0,100,0,3,VALID,"
            "4.5,0,10,0,3,VALID";

        // Store the first entry and destroy the initial logger instance.
        {
            CSVLogger logger(testFile);
            logger.appendToStorage(entry1);
        }

        // Reopen the same file using a separate logger instance.
        {
            CSVLogger logger(testFile);
            logger.appendToStorage(entry2);
        }

        std::ifstream file(testFile);

        ASSERT_TRUE(file.is_open());

        const std::string actualContent(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        const std::string expectedContent =
            expectedHeader + "\n" +
            entry1 + "\n" +
            entry2 + "\n";

        EXPECT_EQ(actualContent, expectedContent);
    }

    // Verify that appending an empty entry adds only a newline.
    TEST_F(CSVLoggerTest, AppendsEmptyEntry)
    {
        CSVLogger logger(testFile);

        const std::string entry;
        logger.appendToStorage(entry);

        std::ifstream file(testFile);

        ASSERT_TRUE(file.is_open());

        const std::string actualContent(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        const std::string expectedContent = expectedHeader + "\n\n";

        EXPECT_EQ(actualContent, expectedContent);
    }

    // Verify that appendToStorage throws std::runtime_error when the file cannot be opened.
    TEST_F(CSVLoggerTest, ThrowsWhenFileCannotBeOpened)
    {
        CSVLogger logger(testFile);

        // Replace the CSV file with a directory to force an open failure.
        ASSERT_TRUE(std::filesystem::remove(testFile));
        ASSERT_TRUE(std::filesystem::create_directory(testFile));

        const std::string entry = "1,OPERATIONAL,NONE";

        EXPECT_THROW(logger.appendToStorage(entry), std::runtime_error);
    }
}

namespace DecodeTests
{
    // Verify that a stored CSV record is reconstructed into the expected LogRecord.
    TEST_F(CSVLoggerTest, DecodesSingleRecord)
    {
        CSVLogger logger(testFile);
        ECUData ecu = createValidECUData();

        // Use the normal logging path to encode and persist the record.
        logger(ecu);

        const LogRecord expectedRecord{
            1, ECUState::OPERATIONAL, FaultCode::NONE,
            {80.0, 0.0, 240.0, 0, 3, SignalState::VALID},
            {2500.0, 0.0, 8000.0, 0, 3, SignalState::VALID},
            {90.0, -40.0, 150.0, 0, 3, SignalState::VALID},
            {12.6, 0.0, 16.0, 0, 3, SignalState::VALID},
            {40.0, 0.0, 100.0, 0, 3, SignalState::VALID},
            {3.0, 0.0, 10.0, 0, 3, SignalState::VALID}
        };

        const auto records = logger.decode();

        ASSERT_EQ(records.size(), 1u);
        EXPECT_EQ(records[0], expectedRecord);
    }

    // Verify that multiple stored records are decoded correctly and in storage order.
    TEST_F(CSVLoggerTest, DecodesMultipleRecords)
    {
        CSVLogger logger(testFile);
        ECUData ecu = createValidECUData();

        logger(ecu);

        ecu.cycleCount = 2;
        ecu.sensors.speed.update(100.0);
        ecu.sensors.rpm.update(3000.0);
        logger(ecu);

        ecu.cycleCount = 3;
        ecu.sensors.speed.update(120.0);
        ecu.sensors.rpm.update(3500.0);
        logger(ecu);

        const LogRecord expectedFirst{
            1, ECUState::OPERATIONAL, FaultCode::NONE,
            {80.0, 0.0, 240.0, 0, 3, SignalState::VALID},
            {2500.0, 0.0, 8000.0, 0, 3, SignalState::VALID},
            {90.0, -40.0, 150.0, 0, 3, SignalState::VALID},
            {12.6, 0.0, 16.0, 0, 3, SignalState::VALID},
            {40.0, 0.0, 100.0, 0, 3, SignalState::VALID},
            {3.0, 0.0, 10.0, 0, 3, SignalState::VALID}
        };

        LogRecord expectedSecond = expectedFirst;
        expectedSecond.cycleCount = 2;
        expectedSecond.speed.value = 100.0;
        expectedSecond.rpm.value = 3000.0;

        LogRecord expectedThird = expectedFirst;
        expectedThird.cycleCount = 3;
        expectedThird.speed.value = 120.0;
        expectedThird.rpm.value = 3500.0;

        const auto records = logger.decode();

        ASSERT_EQ(records.size(), 3u);
        EXPECT_EQ(records[0], expectedFirst);
        EXPECT_EQ(records[1], expectedSecond);
        EXPECT_EQ(records[2], expectedThird);
    }

    // Verify that decoding preserves out-of-range values and their signal states.
    TEST_F(CSVLoggerTest, DecodesOutOfRangeSignals)
    {
        CSVLogger logger(testFile);
        ECUData ecu = createValidECUData();
        ecu.cycleCount = 2;
        ecu.currentState = ECUState::DEGRADED;
        ecu.faultCode = FaultCode::SIGNAL_OUT_OF_LIMITS;
        ecu.sensors.speed.update(300.0);
        ecu.sensors.temperature.update(200.0);
        ecu.sensors.oilPressure.update(-1.0);
        logger(ecu);

        const LogRecord expected{
            2, ECUState::DEGRADED, FaultCode::SIGNAL_OUT_OF_LIMITS,
            {300.0, 0.0, 240.0, 0, 3, SignalState::OUT_OF_RANGE},
            {2500.0, 0.0, 8000.0, 0, 3, SignalState::VALID},
            {200.0, -40.0, 150.0, 0, 3, SignalState::OUT_OF_RANGE},
            {12.6, 0.0, 16.0, 0, 3, SignalState::VALID},
            {40.0, 0.0, 100.0, 0, 3, SignalState::VALID},
            {-1.0, 0.0, 10.0, 0, 3, SignalState::OUT_OF_RANGE}
        };
        const auto records = logger.decode();
        ASSERT_EQ(records.size(), 1u);
        EXPECT_EQ(records[0], expected);
    }

    // Verify that decoding preserves missed-cycle counters and signal availability states.
    TEST_F(CSVLoggerTest, DecodesMissingSignals)
    {
        CSVLogger logger(testFile);
        ECUData ecu = createValidECUData();
        ecu.cycleCount = 6;
        ecu.currentState = ECUState::SAFE_STATE;
        ecu.faultCode = FaultCode::CRITICAL_SENSOR_LOST;
        // Exercise missed-cycle counts below, at, and above the allowed limit.
        for (unsigned int i = 0; i < 5; ++i)
        {
            if (i < 4) ecu.sensors.speed.registerMissedCycle();
            ecu.sensors.temperature.registerMissedCycle();
            if (i < 2) ecu.sensors.batteryVoltage.registerMissedCycle();
            if (i < 3) ecu.sensors.throttle.registerMissedCycle();
        }
        logger(ecu);

        const LogRecord expected{
            6, ECUState::SAFE_STATE, FaultCode::CRITICAL_SENSOR_LOST,
            {80.0, 0.0, 240.0, 4, 3, SignalState::NOT_AVAILABLE},
            {2500.0, 0.0, 8000.0, 0, 3, SignalState::VALID},
            {90.0, -40.0, 150.0, 5, 3, SignalState::NOT_AVAILABLE},
            {12.6, 0.0, 16.0, 2, 3, SignalState::VALID},
            {40.0, 0.0, 100.0, 3, 3, SignalState::VALID},
            {3.0, 0.0, 10.0, 0, 3, SignalState::VALID}
        };
        const auto records = logger.decode();
        ASSERT_EQ(records.size(), 1u);
        EXPECT_EQ(records[0], expected);
    }

    // Verify that all supported ECU state representations are decoded correctly.
    TEST_F(CSVLoggerTest, DecodesECUStates)
    {
        CSVLogger logger(testFile);
        ECUData ecu = createValidECUData();
        const std::array states{ECUState::INIT, ECUState::SELF_TEST,
            ECUState::OPERATIONAL, ECUState::DEGRADED,
            ECUState::SAFE_STATE, ECUState::SHUTDOWN};
        for (const auto state : states)
        {
            ecu.currentState = state;
            logger(ecu);
        }
        const auto records = logger.decode();
        ASSERT_EQ(records.size(), states.size());
        for (std::size_t i = 0; i < states.size(); ++i)
        {
            EXPECT_EQ(records[i].state, states[i]) << "Row: " << i;
        }
    }

    // Verify that all supported fault code representations are decoded correctly.
    TEST_F(CSVLoggerTest, DecodesFaultCodes)
    {
        CSVLogger logger(testFile);
        ECUData ecu = createValidECUData();
        const std::array codes{FaultCode::NONE, FaultCode::CRITICAL_SENSOR_LOST,
            FaultCode::CRITICAL_OUT_OF_LIMITS, FaultCode::SENSOR_DEGRADED,
            FaultCode::SIGNAL_OUT_OF_LIMITS, FaultCode::INCOHERENT_SIGNALS};
        for (const auto code : codes)
        {
            ecu.faultCode = code;
            logger(ecu);
        }
        const auto records = logger.decode();
        ASSERT_EQ(records.size(), codes.size());
        for (std::size_t i = 0; i < codes.size(); ++i)
        {
            EXPECT_EQ(records[i].faultCode, codes[i]) << "Row: " << i;
        }
    }

    // Verify that decoding a file containing only the header returns no records.
    TEST_F(CSVLoggerTest, DecodesHeaderOnlyFile)
    {
        CSVLogger logger(testFile);
        EXPECT_TRUE(logger.decode().empty());
    }

    // Verify that decoding an unknown ECU state throws std::runtime_error.
    TEST_F(CSVLoggerTest, ThrowsWhenECUStateIsUnknown)
    {
        CSVLogger logger(testFile);
        ECUData ecu = createValidECUData();
        ecu.currentState = static_cast<ECUState>(99);
        logger(ecu);
        EXPECT_THROW(logger.decode(), std::runtime_error);
    }

    // Verify that decoding an unknown fault code throws std::runtime_error.
    TEST_F(CSVLoggerTest, ThrowsWhenFaultCodeIsUnknown)
    {
        CSVLogger logger(testFile);
        ECUData ecu = createValidECUData();
        ecu.faultCode = static_cast<FaultCode>(99);
        logger(ecu);
        EXPECT_THROW(logger.decode(), std::runtime_error);
    }

    // Verify that decoding an unknown sensor state throws std::runtime_error.
    TEST_F(CSVLoggerTest, ThrowsWhenSignalStateIsUnknown)
    {
        CSVLogger logger(testFile);
        logger(createValidECUData());
        // Corrupt only the first sensor state while preserving the remaining fields.
        std::string entry = logger.encode();
        const auto position = entry.find(",VALID,");
        ASSERT_NE(position, std::string::npos);
        entry.replace(position, 7, ",UNKNOWN,");
        logger.appendToStorage(entry);
        EXPECT_THROW(logger.decode(), std::runtime_error);
    }

    // Verify that decoding an invalid numeric field throws std::invalid_argument.
    TEST_F(CSVLoggerTest, ThrowsWhenNumericValueIsInvalid)
    {
        CSVLogger logger(testFile);
        logger(createValidECUData());
        std::string entry = logger.encode();
        const auto position = entry.find(',');
        ASSERT_NE(position, std::string::npos);
        // Replace the cycle count with non-numeric text.
        entry.replace(0, position, "INVALID");
        logger.appendToStorage(entry);
        EXPECT_THROW(logger.decode(), std::invalid_argument);
    }

    // Verify that decoding a file with a missing column throws std::out_of_range.
    TEST_F(CSVLoggerTest, ThrowsWhenColumnIsMissing)
    {
        CSVLogger logger(testFile);
        logger(createValidECUData());
        std::string entry = logger.encode();
        std::string header = expectedHeader;
        // Remove the last column from both the header and the record.
        header.erase(header.rfind(','));
        entry.erase(entry.rfind(','));
        std::ofstream file(testFile, std::ios::trunc);
        ASSERT_TRUE(file.is_open());
        file << header << '\n' << entry << '\n';
        file.close();
        ASSERT_FALSE(file.fail());
        EXPECT_THROW(logger.decode(), std::out_of_range);
    }

    // Verify that decoding a missing CSV file throws an exception.
    TEST_F(CSVLoggerTest, ThrowsWhenFileDoesNotExist)
    {
        CSVLogger logger(testFile);
        logger(createValidECUData());
        ASSERT_TRUE(std::filesystem::remove(testFile));
        ASSERT_FALSE(std::filesystem::exists(testFile));
        EXPECT_THROW(logger.decode(), std::exception);
    }

    // Verify that repeated decoding returns consistent records without modifying the CSV file.
    TEST_F(CSVLoggerTest, DecodeDoesNotModifyStorage)
    {
        CSVLogger logger(testFile);
        logger(createValidECUData());
        std::ifstream fileBefore(testFile, std::ios::binary);
        ASSERT_TRUE(fileBefore.is_open());
        const std::string contentBefore(
            (std::istreambuf_iterator<char>(fileBefore)),
            std::istreambuf_iterator<char>());
        fileBefore.close();
        ASSERT_FALSE(contentBefore.empty());

        const auto first = logger.decode();
        const auto second = logger.decode();
        ASSERT_EQ(first.size(), 1u);
        EXPECT_EQ(first, second);

        std::ifstream fileAfter(testFile, std::ios::binary);
        ASSERT_TRUE(fileAfter.is_open());
        const std::string contentAfter(
            (std::istreambuf_iterator<char>(fileAfter)),
            std::istreambuf_iterator<char>());
        EXPECT_EQ(contentBefore, contentAfter);
    }
}

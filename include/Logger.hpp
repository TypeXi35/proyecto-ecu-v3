#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "ControlData.hpp"

struct SensorInfo {
    double       value{};
    double       minValue{};
    double       maxValue{};
    unsigned int missedCycles{};
    unsigned int maxMissedCycles{};
    SignalState  state{};

    bool operator==(const SensorInfo&) const = default;
};

struct LogRecord {
    unsigned int cycleCount{};
    ECUState     state{};
    FaultCode    faultCode{};
    SensorInfo   speed{};
    SensorInfo   rpm{};
    SensorInfo   temperature{};
    SensorInfo   batteryVoltage{};
    SensorInfo   throttle{};
    SensorInfo   oilPressure{};

    bool operator==(const LogRecord&) const = default;
};

class Logger {
    public:
        explicit Logger(std::filesystem::path file) : filePath(std::move(file)) {}
        virtual ~Logger() = default;

        void operator()(const ECUData& ecu) {
            updateData(ecu);
            appendToStorage(encode());
        }

        // An entry is the encoded form of one record. std::string is used as a byte
        // buffer: it may be text (CSV) or raw bytes, including '\0' (e.g. binary/encrypted).

        // Turns the current record into one entry (no side effects).
        virtual std::string encode() const = 0;
        // Writes an already-encoded entry; does not depend on encode() having run.
        virtual void appendToStorage(const std::string& entry) = 0;
        virtual std::vector<LogRecord> decode() const = 0;

    protected:
        const LogRecord& record() const { return current; }
        const std::filesystem::path filePath;

    private:
        void updateData(const ECUData& ecu);

        LogRecord current{};
};

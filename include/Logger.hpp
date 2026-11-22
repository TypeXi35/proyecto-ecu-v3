#pragma once

#include <filesystem>
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
            encode();
            appendToStorage();
        }

        virtual std::vector<LogRecord> decode() const = 0;

    protected:
        const LogRecord& record() const { return current; }
        const std::filesystem::path filePath;

    private:
        void updateData(const ECUData& ecu);
        virtual void encode() = 0;
        virtual void appendToStorage() = 0;

        LogRecord current{};
};

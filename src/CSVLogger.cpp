#include "CSVLogger.hpp"
#include "EnumText.hpp"

#include <array>
#include <cstddef>
#include <format>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "rapidcsv.h"

namespace {
    // Column prefixes, in the same order as the SensorInfo members of LogRecord.
    constexpr std::array<std::string_view, 6> sensorNames{
        "speed", "rpm", "temperature", "batteryVoltage", "throttle", "oilPressure"
    };

    // Column suffixes, in the same order as the members of SensorInfo.
    constexpr std::array<std::string_view, 6> sensorFields{
        "value", "min", "max", "missedCycles", "maxMissedCycles", "state"
    };

    std::string headerRow()
    {
        std::string header = "cycleCount,state,faultCode";
        for (auto sensor : sensorNames) {
            for (auto field : sensorFields) {
                header += ',';
                header += sensor;
                header += '_';
                header += field;
            }
        }
        return header;
    }

    // Converts a text cell back to its enum; a name not in the table means a corrupted log.
    template <typename E, std::size_t N>
    E readEnum(const rapidcsv::Document& doc, const std::string& column, std::size_t row,
               const EnumTable<E, N>& table)
    {
        const std::string text = doc.GetCell<std::string>(column, row);
        if (const auto value = fromText(text, table)) {
            return *value;
        }
        throw std::runtime_error(std::format(
            "CSVLogger: unknown value '{}' in column '{}', data row {}", text, column, row));
    }

    SensorInfo readSensor(const rapidcsv::Document& doc, std::string_view sensor, std::size_t row)
    {
        auto column = [sensor](std::string_view field) {
            return std::format("{}_{}", sensor, field);
        };

        return SensorInfo{
            doc.GetCell<double>(column("value"), row),
            doc.GetCell<double>(column("min"), row),
            doc.GetCell<double>(column("max"), row),
            doc.GetCell<unsigned int>(column("missedCycles"), row),
            doc.GetCell<unsigned int>(column("maxMissedCycles"), row),
            readEnum(doc, column("state"), row, signalStateNames)
        };
    }
}

CSVLogger::CSVLogger(std::filesystem::path file) : Logger(std::move(file))
{
    // Write the header only for a new (or empty) file, so existing logs keep growing.
    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec) || std::filesystem::file_size(filePath, ec) == 0) {
        std::ofstream out(filePath);
        out << headerRow() << '\n';
    }
}

std::string CSVLogger::encode() const
{
    const LogRecord& r = record();

    std::string line = std::format("{},{},{}",
        r.cycleCount,
        toText(r.state, ecuStateNames),
        toText(r.faultCode, faultCodeNames));

    // Same order as sensorNames, so each value lands under its header column.
    for (const SensorInfo* s : {&r.speed, &r.rpm, &r.temperature,
                                &r.batteryVoltage, &r.throttle, &r.oilPressure}) {
        // Same order as sensorFields.
        std::format_to(std::back_inserter(line), ",{},{},{},{},{},{}",
            s->value,
            s->minValue,
            s->maxValue,
            s->missedCycles,
            s->maxMissedCycles,
            toText(s->state, signalStateNames));
    }
    return line;
}

void CSVLogger::appendToStorage(const std::string& entry)
{
    std::ofstream out(filePath, std::ios::app);
    if (!out) {
        throw std::runtime_error(std::format("CSVLogger: cannot open '{}' for writing", filePath.string()));
    }
    out << entry << '\n';
}

std::vector<LogRecord> CSVLogger::decode() const
{
    // Row 0 holds the column names; there is no row-label column.
    const rapidcsv::Document doc(filePath.string(), rapidcsv::LabelParams(0, -1));

    std::vector<LogRecord> records;
    records.reserve(doc.GetRowCount());

    for (std::size_t row = 0; row < doc.GetRowCount(); ++row) {
        records.push_back(LogRecord{
            doc.GetCell<unsigned int>("cycleCount", row),
            readEnum(doc, "state", row, ecuStateNames),
            readEnum(doc, "faultCode", row, faultCodeNames),
            readSensor(doc, sensorNames[0], row),
            readSensor(doc, sensorNames[1], row),
            readSensor(doc, sensorNames[2], row),
            readSensor(doc, sensorNames[3], row),
            readSensor(doc, sensorNames[4], row),
            readSensor(doc, sensorNames[5], row)
        });
    }
    return records;
}

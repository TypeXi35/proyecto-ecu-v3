#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Logger.hpp"

class CSVLogger : public Logger{
    public:
        explicit CSVLogger(std::filesystem::path file);
        std::string encode() const override;
        void appendToStorage(const std::string& entry) override;
        std::vector<LogRecord> decode() const override;
};
#pragma once

#include <string>
#include <vector>

#include "Logger.hpp"

class CSVLogger : public Logger{
    public:
        using Logger::Logger;
        std::vector<LogRecord> decode() const override;
    private:
        void encode() override;
        void appendToStorage() override;

        std::string line;
};
#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>

#include "ControlData.hpp"
#include "ECUState.hpp"
#include "SignalTypes.hpp"

// Text names for the enums written to the logs. Each table is used in both
// directions (enum -> text and text -> enum), so writing and reading stay in sync.
// Names match the enumerators exactly so a log can be searched with the code's names.

template <typename E, std::size_t N>
using EnumTable = std::array<std::pair<E, std::string_view>, N>;

inline constexpr EnumTable<ECUState, 6> ecuStateNames{{
    {ECUState::INIT,        "INIT"},
    {ECUState::SELF_TEST,   "SELF_TEST"},
    {ECUState::OPERATIONAL, "OPERATIONAL"},
    {ECUState::DEGRADED,    "DEGRADED"},
    {ECUState::SAFE_STATE,  "SAFE_STATE"},
    {ECUState::SHUTDOWN,    "SHUTDOWN"},
}};

inline constexpr EnumTable<FaultCode, 6> faultCodeNames{{
    {FaultCode::NONE,                   "NONE"},
    {FaultCode::CRITICAL_SENSOR_LOST,   "CRITICAL_SENSOR_LOST"},
    {FaultCode::CRITICAL_OUT_OF_LIMITS, "CRITICAL_OUT_OF_LIMITS"},
    {FaultCode::SENSOR_DEGRADED,        "SENSOR_DEGRADED"},
    {FaultCode::SIGNAL_OUT_OF_LIMITS,   "SIGNAL_OUT_OF_LIMITS"},
    {FaultCode::INCOHERENT_SIGNALS,     "INCOHERENT_SIGNALS"},
}};

inline constexpr EnumTable<SignalState, 3> signalStateNames{{
    {SignalState::VALID,         "VALID"},
    {SignalState::OUT_OF_RANGE,  "OUT_OF_RANGE"},
    {SignalState::NOT_AVAILABLE, "NOT_AVAILABLE"},
}};

template <typename E, std::size_t N>
constexpr std::string_view toText(E value, const EnumTable<E, N>& table)
{
    for (const auto& [e, name] : table) {
        if (e == value) return name;
    }
    return "UNKNOWN";
}

template <typename E, std::size_t N>
constexpr std::optional<E> fromText(std::string_view text, const EnumTable<E, N>& table)
{
    for (const auto& [e, name] : table) {
        if (name == text) return e;
    }
    return std::nullopt;
}

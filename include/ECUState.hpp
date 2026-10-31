#pragma once

enum class ECUState {
    INIT,
    SELF_TEST, 
    OPERATIONAL,
    DEGRADED,
    SAFE_STATE,
    SHUTDOWN
};
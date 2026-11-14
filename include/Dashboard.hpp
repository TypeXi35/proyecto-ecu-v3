#pragma once

#include <map>
#include <string>
#include <ftxui/component/app.hpp>

#include "ControlData.hpp"
#include "ECUState.hpp"

class Dashboard {
    private:
        inline static auto const stateLabels = []{
            using enum ECUState;
            return std::map<const ECUState, std::string>({
                // {OFF, "off"},
                {INIT, "init"},
                {SELF_TEST, "self-test"},
                {OPERATIONAL, "operational"},
                {DEGRADED, "degraded"},
                {SAFE_STATE, "safe state"},
                {SHUTDOWN, "shutdown"}
            });
        }();
        ECUData currentEcu{};
        ftxui::App screen;

    public:
        Dashboard();
        ~Dashboard();

        void postUpdate(const ECUData& ecu);
        void renderLoop();
};

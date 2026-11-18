#include "Dashboard.hpp"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <ftxui/ftxui.hpp>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <array>
#include <ControlData.hpp>

Dashboard::Dashboard(): screen(ftxui::ScreenInteractive::FullscreenAlternateScreen()) {};
Dashboard::~Dashboard() {
    screen.Exit();
}

void Dashboard::postUpdate(const ECUData& ecu) {
    currentEcu = ecu;
    screen.RequestAnimationFrame();
}

void Dashboard::renderLoop() {
    using namespace ftxui; // scoped 😉
    screen.Loop(Renderer([this] {
        auto sensors = std::array<Sensor*, 6> {
            &this->currentEcu.sensors.batteryVoltage,
            &this->currentEcu.sensors.oilPressure,
            &this->currentEcu.sensors.rpm,
            &this->currentEcu.sensors.speed,
            &this->currentEcu.sensors.temperature,
            &this->currentEcu.sensors.throttle
        } | std::views::transform([](Sensor *sensor) {
            double const value = sensor->getValue();
            double const min = sensor->getMin();
            double const max = sensor->getMax();
            double const rate = (value - min)/(max - min);

            Decorator const couleur = color([&sensor]{
                using enum SignalState;
                using enum ftxui::Color::Palette256;
                switch (sensor->getState()) {
                    case NOT_AVAILABLE: return Yellow1;
                    case OUT_OF_RANGE: return Red1;
                    case VALID: default: return SeaGreen1;
                }
            }());

            return hbox(
                text(std::format("{} ", signalText(sensor->getId()))),
                gauge(rate) | couleur,
                text(std::format(" {:.1f}{}", value, sensor->getUnit())) | couleur
            );
        });

        auto ecuState = this->currentEcu.currentState;
        auto stateLabel = stateLabels.at(ecuState);
        auto sensorBox = vbox(std::ranges::to<std::vector<Element>>(sensors));
        auto document = vbox(
            sensorBox,
            hbox(
                hbox() | xflex,
                text(std::format("\nState: {}", stateLabel)) | color([&ecuState]{
                    using enum ECUState;
                    using enum ftxui::Color::Palette256;
                    switch (ecuState) {
                        case DEGRADED: return Yellow1;
                        case SAFE_STATE: return Red1;
                        default: return Grey0;
                    }
                }()),
                hbox() | xflex
            )
        );
        return vbox(
            vbox() | yflex,
            window(text("ECU VEHICLE DASHBOARD"), document) | borderDashed,
            vbox() | yflex
        );
    }));
}

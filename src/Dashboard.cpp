#include "Dashboard.hpp"

#include <format>
#include <ftxui/ftxui.hpp>
#include <ranges>
#include <array>
#include <ControlData.hpp>
#include <cmath>
#include <numbers> // Requires C++20

Dashboard::Dashboard(): screen(ftxui::ScreenInteractive::FullscreenAlternateScreen()) {};
Dashboard::~Dashboard() {
    screen.Exit();
}

void Dashboard::postUpdate(const ECUData& ecu) {
    currentEcu = ecu;
    screen.RequestAnimationFrame();
}

ftxui::Color sensorColour(SignalState sensorState) {
    using enum SignalState;
    using enum ftxui::Color::Palette256;
    switch (sensorState) {
        case NOT_AVAILABLE: return Yellow1;
        case OUT_OF_RANGE: return Red1;
        case VALID: default: return SeaGreen1;
    }
}

ftxui::Canvas arc(Sensor& sensor, int size = 50) {
    static double const pi = std::numbers::pi;
    int const width = size; int const height = width/2;
    int const cx = height; int const cy = height; int const radius = height;
    ftxui::Canvas c(width, height);

    double const value = sensor.getValue();
    double const min = sensor.getMin();
    double const max = sensor.getMax();
    double const rate = (value - min)/(max - min);
    auto const color = sensorColour(sensor.getState());
    
    double const startAngle = pi;
    // [1,2) will be a 
    double const endAngle = (1+rate) * pi;
    int const segments = radius * 4;

    int prevX = cx + static_cast<int>(std::round(radius * std::cos(startAngle)));
    int prevY = cy + static_cast<int>(std::round(radius * std::sin(startAngle)));

    for (int i = 1; i <= segments; ++i) {
      double angle = startAngle + (endAngle - startAngle) * i / segments;
      int x = cx + static_cast<int>(std::round(radius * std::cos(angle)));
      int y = cy + static_cast<int>(std::round(radius * std::sin(angle)));
      c.DrawPointLine(prevX, prevY, x, y, color);
      prevX = x;
      prevY = y;
    }
    // kinda horizontally centered
    c.DrawText(width/3, cy-2, std::format("{:.1f} {}", value, sensor.getUnit()));
    return c;
}


void Dashboard::renderLoop() {
    using namespace ftxui; // scoped 😉
    screen.Loop(Renderer([this] {
        auto sensors = std::array<Sensor*, 4> {
            &this->currentEcu.sensors.batteryVoltage,
            &this->currentEcu.sensors.oilPressure,
            &this->currentEcu.sensors.temperature,
            &this->currentEcu.sensors.throttle
        } | std::views::transform([](Sensor *sensor) {
            double const value = sensor->getValue();
            double const min = sensor->getMin();
            double const max = sensor->getMax();
            double const rate = (value - min)/(max - min);

            Decorator const couleur = color(sensorColour(sensor->getState()));

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
            hbox(
                vbox(
                    vbox() | yflex,
                    canvas(arc(this->currentEcu.sensors.rpm)),
                    vbox() | yflex,
                    canvas(arc(this->currentEcu.sensors.speed)),
                    vbox() | yflex
                ) | xflex,
                hbox() | xflex,
                vbox(
                    vbox() | yflex,
                    sensorBox,
                    vbox() | yflex
                ) | xflex
            ),
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

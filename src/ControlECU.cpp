#include "ControlData.hpp"
#include "Sensor.hpp"
#include "ECUState.hpp"
#include "SignalTypes.hpp"

#include <vector>
#include <unordered_map>

constexpr int SPEED_PER_RPM = 0.04;
constexpr int SPEED_TOLERANCE = 15.0;

class ControlECU{
    private:
        ECUState currentState;
        ECUState proposedTransition;
        ECUData data;
        std::unordered_map<SignalId,const Sensor*> sensorMap;
    public:
        ControlECU(const std::vector<Sensor>& gateway_sensors): 
            currentState(ECUState::INIT),
            data{
                {
                    getSensor(gateway_sensors, SignalId::SPEED),
                    getSensor(gateway_sensors, SignalId::RPM),
                    getSensor(gateway_sensors, SignalId::TEMPERATURE),
                    getSensor(gateway_sensors, SignalId::BATTERY_VOLTAGE),
                    getSensor(gateway_sensors, SignalId::THROTTLE),
                    getSensor(gateway_sensors, SignalId::OIL_PRESSURE)
                },
                currentState
            }
        {
        }
        void runControlCycle(){
            switch(currentState){
                case ECUState::INIT:
                    stateTransition(ECUState::OPERATIONAL);
                    break;
                case ECUState::OPERATIONAL:
                    if(hasCriticalFault()){
                        stateTransition(ECUState::SAFE_STATE);
                        break;
                    }
                    if(isDegraded()){
                        stateTransition(ECUState::DEGRADED);
                    }
                    break;
                case ECUState::DEGRADED:
                    if(hasCriticalFault()){
                        stateTransition(ECUState::SAFE_STATE);
                        break;
                    }
                    if(isDegraded() == false){
                        stateTransition(ECUState::OPERATIONAL);
                    }
                    break;
                case ECUState::SAFE_STATE:
                    break;
            }
        }
    private:
        Sensor& getSensorByName(const std::string& name);
        bool hasCriticalFault(){
            int expectedSpeed = data.sensors.rpm.getValue() * SPEED_PER_RPM;
            if(data.sensors.temperature.getState() != SignalState::VALID && data.sensors.temperature.getState() != SignalState::NOT_AVAILABLE){
                return true;
            }

            if(data.sensors.temperature.getMissedCycles() >= 3 || data.sensors.batteryVoltage.getMissedCycles() >= 3){
                return true;
            }
            bool coherent = std::abs(data.sensors.speed.getValue() - expectedSpeed) <= SPEED_TOLERANCE;
            if(!coherent){
                return true;
            }
            return false;
        }
        bool isDegraded(){
            
        }
        void stateTransition(ECUState newState){
            currentState = newState;
        }
        ECUData getControlData(){
            return data;
        }
        static const Sensor& getSensor(
            const std::vector<Sensor>& sensors,
            SignalId id)
            {
                for (const Sensor& sensor : sensors) {
                    if (sensor.getId() == id) {
                        return sensor;
                    }
                }
            }
};


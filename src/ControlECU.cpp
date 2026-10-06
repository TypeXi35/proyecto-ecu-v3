#include <Sensor.hpp>
#include <ECUState.hpp>
#include <SignalTypes.hpp>

#include <vector>
#include <unordered_map>

constexpr int SPEED_PER_RPM = 0.04;
constexpr int SPEED_TOLERANCE = 15.0;

class ControlECU{
    private:
        ECUState currentState;
        ECUState proposedTransition;
        std::unordered_map<SignalId,const Sensor*> sensorMap;
    public:
        ControlECU(std::vector<Sensor>& gateway_sensors): 
            currentState(ECUState::INIT)
        {   
            for(Sensor& sensor : gateway_sensors){
                sensorMap.emplace(sensor.getId(), &sensor);
            }
        };
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
            const Sensor* temperature = sensorMap.at(SignalId::TEMPERATURE);
            const Sensor* voltage = sensorMap.at(SignalId::BATTERY_VOLTAGE);
            const Sensor* rpm = sensorMap.at(SignalId::RPM);
            const Sensor* speed = sensorMap.at(SignalId::SPEED);
            int expectedSpeed = rpm->getValue() * SPEED_PER_RPM;
            if(temperature->getState() != SignalState::VALID && temperature->getState() != SignalState::NOT_AVAILABLE){
                return true;
            }

            if(temperature->getMissedCycles() >= 3 || voltage->getMissedCycles() >= 3){
                return true;
            }
            bool coherent = std::abs(speed->getValue() - expectedSpeed) <= SPEED_TOLERANCE;
            if(coherent){
                return true;
            }
        }
        bool isDegraded(){
            
        }
        const std::unordered_map<SignalId, const Sensor*>& getSensorMap(){
            return sensorMap;
        }
        void stateTransition(ECUState newState){
            currentState = newState;
        }
};


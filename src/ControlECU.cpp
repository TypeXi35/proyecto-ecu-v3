#include <Sensor.hpp>
#include <ECUState.hpp>

#include <vector>

class ControlECU{
    private:
        std::vector<Sensor>& sensors;
        ECUState currentState;
        ECUState signalState;
        ECUState missingSignalsState;
        ECUState coherenceState;

    public:
        ControlECU(std::vector<Sensor>& gateway_sensors): 
            sensors(gateway_sensors),
            currentState(ECUState::INIT),
            signalState(ECUState::INIT),
            missingSignalsState(ECUState::INIT),
            coherenceState(ECUState::INIT)
        {   
        };
        void runControlCycle(){
            switch(currentState){
                case ECUState::INIT:
                checkSignalState();
                break;
                case ECUState::OPERATIONAL:
                break;
                case ECUState::DEGRADED:
                break;
                case ECUState::SAFE_STATE:

            }
        }
    private:
        ECUState checkSignalState(){

        };
        ECUState checkMissingSignals(){

        };
        ECUState checkCoherence()
        {

        };

};


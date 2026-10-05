#include <ControlECU.hpp>

int main(){
    VehicleSimulator Vehicle;
    GatewayECU Gateway(Vehicle.exposeSignals()); // Señales simples
    Gateway.processSignals();
    ControlECU control(Gateway.exposeSensors());
    Dashboard(Gateway.exposeSensors()); 
    //INIT
    control.Run();
    Dashboard.Draw(control.exposeData());
    if(control.exposeState() == SAFE_STATE){
        return 1;
    }
    else{
    }
    while(currentState != SAFE_STATE){
        GatewayECU.processSignals(Vehicle.exposeSignals());
        ControlECU.updateSensors(GatewayECU.exposeSensors());
        control.Run();
        Dashboard.Draw();
    }
    return 1;
    }
}
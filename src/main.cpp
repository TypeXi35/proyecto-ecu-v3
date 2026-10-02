

int main(){
    VehicleSimulator Vehicle;
    GatewayECU Gateway(Vehicle.exposeSignals()); // Señales simples
    Gateway.processSignals()
    ControlECU Control(Gateway.exposeSensors());
    Dashboard(Gateway.exposeSensors()); 
    //INIT
    Control.Run();
    Dashboard.Draw(Control.exposeData());
    if(Control.exposeState() == SAFE_STATE){
        return 1;
    }
    else{
    }
    while(currentState != SAFE_STATE){
        GatewayECU.processSignals(Vehicle.exposeSignals());
        ControlECU.updateSensors(GatewayECU.exposeSensors());
        Control.Run();
        Dashboard.Draw();
    }
    return 1;
    }
}
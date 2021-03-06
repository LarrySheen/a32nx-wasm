#pragma once
#include "../common_sys.h"


class PressSys {
private:
    bool takeoff;
    bool landed;
    double landing_timer;
    double man_auto_timer;

    const int max_normal_cabin_alt = 8000;   //feet
    const int max_operating_alt = 39800;     //feet
    const int cab_alt_warn = 9200;
    const int outflow_max_alt = 15000;
    const int ldg_max_cabin_vs = 750;
    const int norm_max_cabin_vs = 2000;
    const double ofv_inc_rate = 3.33;                //3.33%/s
    const double ofv_dec_rate = -3.33;
    const double atm_air_moles_per_mass = 34.518;   //moles/kg
    
    const double toga_ldg_delta = 0.1;         //+0.1 deltaP
    double cabin_vs_target;
    double pack_flow;
    double out_flow;
    double max_cabin_vs;
    int active_CPC;                         //0 = SYS1, 1 = SYS2

    const double fuselage_volume = 192;     //CKPT = 9, CABIN = 139, CARGO = 15.5, 28 cubic meters
    double cabin_air_mass;
    double cabin_pressure;
    double last_cabin_pressure;

    PIDClass outflow_controller;

    void calculatePackFlow() {
        if(lSimVarsValue[DUCT1] && lSimVarsValue[DUCT2]){
            if (lSimVarsValue[PACK1_VALVE] && lSimVarsValue[PACK2_VALVE]) {
                const int pack_controller = lSimVarsValue[PACK_FLOW_CONTROLLER];
                switch (pack_controller)
                {
                    case 0: {
                        //CKPT  0.069+0.065     kg/s
                        //CABIN 0.582+0.548
                        //CARGO 0.088+0.132
                        const double fresh_flow = 0.651;
                        const double recirc_flow = 0.833;
                        pack_flow = fresh_flow + recirc_flow;
                        break;
                    }
                    case 1: {
                        //CKPT  0.086+0.062     kg/s
                        //CABIN 0.722+0.522
                        //CARGO 0.088+0.132
                        const double fresh_flow = 0.808;
                        const double recirc_flow = 0.804;
                        pack_flow = fresh_flow + recirc_flow;
                        break;
                    }
                    case 2: {
                        //CKPT  0.103+0.059     kg/s
                        //CABIN 0.872+1.501
                        //CARGO 0.088+0.132
                        const double fresh_flow = 0.975;
                        const double recirc_flow = 1.78;
                        pack_flow = fresh_flow + recirc_flow;
                        break;
                    }
                    default:
                        break;
                }
            }
        } else if(lSimVarsValue[DUCT1] || lSimVarsValue[DUCT2]){
            //CKPT  0.050+0.067
            //CABIN 0.437+0.577
            //CARGO 0.088+0.132
            const double fresh_flow = 0.487;
            const double recirc_flow = 0.864;
            pack_flow = fresh_flow + recirc_flow;
        } else {
            pack_flow = 0;
        }
    }

    void calculateOutFlow() {
        const double ditching  = !lSimVarsValue[DITCH];
        const double max_packflow = 2.755;
        double safety_outflow = 0;
        if (lSimVarsValue[DELTA_PRESSURE] >= 8.6 || lSimVarsValue[DELTA_PRESSURE] < -0.5) {
            safety_outflow = lSimVarsValue[DELTA_PRESSURE] * 2.755;
            lSimVarsValue[SAFETY_1] = 1;
            lSimVarsValue[SAFETY_2] = 1;
        } else {
            lSimVarsValue[SAFETY_1] = 0;
            lSimVarsValue[SAFETY_2] = 0;
        }
        const double max_outFlow = lSimVarsValue[DELTA_PRESSURE] * max_packflow;
        out_flow = (lSimVarsValue[OUTFLOW_VALVE]/100) * ditching * max_outFlow + safety_outflow;
    }

    void setCabinAlt(double alt) {
        lSimVarsValue[CABIN_ALTITUDE_GOAL] = alt;
    }

    void incCabinVS() {
        //to increase cabin VS the outflow valve close
        lSimVarsValue[OUTFLOW_VALVE] += ofv_inc_rate * deltaT * 0.001;
    }

    void decCabinVS(){
        //to decrease cabin VS the outflow valve opens
        lSimVarsValue[OUTFLOW_VALVE] -= ofv_dec_rate * deltaT * 0.001;
    }

    void setLDGELEV(double alt) {
    //need to get feild elev from cdu f-pln
    }

    void setMaxVS() {
        if ((aSimVarsValue[GEAR_POS] == 2 || aSimVarsValue[ALTITUDE_ABV_GND] <= 5000) && aSimVarsValue[CURRENT_VSPEED] < 0) {
            max_cabin_vs = ldg_max_cabin_vs;
        } else {
            max_cabin_vs = norm_max_cabin_vs;
        }
    }
    void autoSetCabinAltTarget() {
        lSimVarsValue[CABIN_ALTITUDE_GOAL] = ((aSimVarsValue[ALTITUDE] + aSimVarsValue[CURRENT_VSPEED]) * max_normal_cabin_alt/max_operating_alt);
    }

    void autoSetCabinVS() {
        cabin_vs_target = max_cabin_vs * (lSimVarsValue[CABIN_ALTITUDE_GOAL] - lSimVarsValue[CABIN_ALTITUDE]) / max_normal_cabin_alt;
    }

    void pressureToCabVS() {
        lSimVarsValue[CABIN_ALTITUDE_RATE] = (altitude_AtPressure(cabin_pressure) - altitude_AtPressure(last_cabin_pressure)) / (60 * deltaT * 0.001);
    }

    double cabVSToDeltaP(const double cabVS) {
        //gives rate of presssure change/s required from cabvs demand 
        return pressure_AtAltitude((lSimVarsValue[CABIN_ALTITUDE] + cabVS)) - aSimVarsValue[AMB_PRESS];
    }

    double calculatedeltaP(const double cab_altitude) {
        return pressure_AtAltitude(cab_altitude) - aSimVarsValue[AMB_PRESS];
    }

    double outflow_control_Error() {
        //uses different parameters based on current state
        if (lSimVarsValue[DELTA_PRESSURE] >= 8) {
            return (7.9 - lSimVarsValue[DELTA_PRESSURE]);
        }
        if (lSimVarsValue[DELTA_PRESSURE] <= -0.5) {
            return (-0.4 - lSimVarsValue[DELTA_PRESSURE]);
        }
        if ((aSimVarsValue[ENG1_THROTTLE] >= 95 || aSimVarsValue[ENG2_THROTTLE] >= 95) && aSimVarsValue[ON_GROUND]) {
            landing_timer = 0;
            return (toga_ldg_delta - lSimVarsValue[DELTA_PRESSURE]);
        }
        if (aSimVarsValue[ON_GROUND] && !landed) {
            landed = true;
        }
        if (landed && landing_timer <= 55) {
            landing_timer += deltaT * 0.001;
            return toga_ldg_delta - lSimVarsValue[DELTA_PRESSURE];
        }
        if (aSimVarsValue[CURRENT_VSPEED] > 50 || aSimVarsValue[CURRENT_VSPEED] < 50) {
            return cabVSToDeltaP(cabin_vs_target) - cabVSToDeltaP(lSimVarsValue[CABIN_ALTITUDE_RATE]);
        }
        return calculatedeltaP(lSimVarsValue[CABIN_ALTITUDE_GOAL]) - lSimVarsValue[DELTA_PRESSURE];
    }

    void closeOutflow() {
        if (lSimVarsValue[OUTFLOW_VALVE] > 0) {
            lSimVarsValue[OUTFLOW_VALVE] -= ofv_dec_rate;
        }
    }

public:
    void init() {
        for (int i = OUTFLOW_VALVE; i <= CPC_SYS2; i++) {
            lSimVarsValue[i] = 0;
        }
        takeoff = false;
        landed = false;
        lSimVarsValue[OUTFLOW_VALVE] = 100;
        lSimVarsValue[CPC_SYS1] = 0;
        lSimVarsValue[CPC_SYS2] = 0;
        lSimVarsValue[CABIN_ALTITUDE] = aSimVarsValue[ALTITUDE];
        lSimVarsValue[CABIN_ALTITUDE_GOAL] = aSimVarsValue[ALTITUDE];
        cabin_pressure = pressure_AtAltitude(lSimVarsValue[CABIN_ALTITUDE]);
        cabin_air_mass = atm_air_moles_per_mass * idealGasMoles(cabin_pressure, fuselage_volume, lSimVarsValue[FWD_TEMP]+273.15);
        outflow_controller.init(0.05, 0.001, 0.01, lastAbsTime, ofv_inc_rate, ofv_dec_rate);
        man_auto_timer = 0;
    }
    void update(const double currentAbsTime) {
        
        setMaxVS();
        //switching of the CPC_SYS
        if (lSimVarsValue[MAN_CAB_PRESS] && man_auto_timer <= 10) {
            man_auto_timer += deltaT*0.001;
            lSimVarsValue[CPC_SYS1] = 0;
            lSimVarsValue[CPC_SYS2] = 0;
        } else if(!(lSimVarsValue[MAN_CAB_PRESS])){
            man_auto_timer = 0;
            active_CPC ^= 1;
            switch (active_CPC)
            {
                case 0:
                    lSimVarsValue[CPC_SYS1] = 1;
                    break;
                case 1:
                    lSimVarsValue[CPC_SYS2] = 1;
                    break;
                default:
                    break;
            }
            autoSetCabinVS();
        }
        
        if (lSimVarsValue[MAN_LDG_ELEV_PCT] == 0) {
            autoSetCabinAltTarget();
        } else {
            setCabinAlt(lSimVarsValue[MAN_LAND_ELEV]);
        }

        if (lSimVarsValue[MAN_VS_CTRL] == 0) {
            incCabinVS();
        } else if (lSimVarsValue[MAN_VS_CTRL] == 2) {
            decCabinVS();
        }
        
        lSimVarsValue[DELTA_PRESSURE] = calculatedeltaP(lSimVarsValue[CABIN_ALTITUDE]);
        
        if (lSimVarsValue[CPC_SYS1] || lSimVarsValue[CPC_SYS2]) {
            const double error = outflow_control_Error();
            const double outflow_control = lSimVarsValue[OUTFLOW_VALVE] + outflow_controller.control(error, currentAbsTime);
            if (outflow_control >= 0 && outflow_control <= 100) {
                lSimVarsValue[OUTFLOW_VALVE] = outflow_control;
            }
        }
        if (lSimVarsValue[CABIN_ALTITUDE] >= 15000) {
            closeOutflow();
        }
        calculatePackFlow();
        calculateOutFlow();
        cabin_air_mass += (pack_flow - out_flow) * deltaT * 0.001;
        const double cabin_air_moles = cabin_air_mass * atm_air_moles_per_mass;
        cabin_pressure = idealGasPressure(fuselage_volume, lSimVarsValue[FWD_TEMP]+273, cabin_air_moles);
        lSimVarsValue[CABIN_ALTITUDE] = altitude_AtPressure(cabin_pressure);
    }
};
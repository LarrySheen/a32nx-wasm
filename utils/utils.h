#pragma once
#ifndef UTILS
#define UTILS

#include "PID.h"
#include "bit_oper.h"
#include <math.h>
#include <chrono>

#ifndef DEBUG
#define DEBUG
#endif // !DEBUG
/*
* ============== *
* UTIL FUNCTIONS *
* ============== *
*/
uint64_t timeSinceEpoch();
double convert_rankineToCelcius(const double rankine);
double convert_inHgToPSI(const double inHg);
double convert_PaToPSI(const double Pa);
double convert_feetToMeters(const double feet);
double convert_metersToFeet(const double meters);

uint64_t timeSinceEpoch() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

double convert_rankineToCelcius(const double rankine) {
    return ((rankine - 491.67) * (0.5556));
}
double convert_inHgToPSI(const double inHg) {
    return (inHg * 0.491);
}

double convert_PaToPSI(const double Pa) {
    return (Pa * 0.000145038);
}

double convert_feetToMeters(const double feet) {
    return (feet * 0.3048);
}

double convert_metersToFeet(const double meters) {
    return (meters * 3.28084);
}

double temperature_AtAltitude(const double amb_temp, const double amb_alt, const double target_alt) {
    //-0.0019812 is temp lapse rate in degree/ft inc
    return (amb_temp + (0.0019812 * (amb_alt - target_alt))); 
}

double pressure_AtAltitude(const double target_alt) {
    /* Barometric formula
     * P = Pb * e^(-go * M * h/ R * T)
     * 14.696 reference pressure in PSI
     * 293.15K is reference temp(std)
     * go       = 9.80665 m/s^2            32.17405 ft/s2
     * M        = 0.0289644 kg/mol         28.9644 lb/lb-mol
     * R        = 8.3144598 J/(mol�K)      8.9494596�10^4 lb�ft2
     * constant = 0.0341626203            -0.0104129422
     * h = target_alt
    */ 
    const int barometeric_constant = -0.0104129422;
    const int ref_pressure = 14.696;
    return (ref_pressure * exp(target_alt * barometeric_constant));
}

double altitude_AtPressure(const double pressure) {
    /* Barometric formula
     * P = Pb * e^(-go * M * h/ R * T)
     * 14.696 reference pressure in PSI
     * 293.15K is reference temp(std)
     * go       = 9.80665 m/s^2            32.17405 ft/s2
     * M        = 0.0289644 kg/mol         28.9644 lb/lb-mol
     * R        = 8.3144598 J/(mol�K)      8.9494596�10^4 lb�ft2
     * constant = 0.0341626203            -0.0104129422
     * h = target_alt
    */
    const int barometric_constant_inv = -96.0343369619;
    const int ref_pressure = 14.696;
    return   (log(pressure / ref_pressure) * barometric_constant_inv);
}

double idealGasPressure(const double volume, const double temp, const double moles) {
    const int ideal_gas_constant = 8.314462618;
    return (moles*temp*ideal_gas_constant/volume);
}

double idealGasMoles(const double pressure, const double volume, const double temp) {
    const int ideal_gas_constant = 8.314462618;
    return (pressure * volume / (temp * ideal_gas_constant));
}

void debug_print(const char* message) {
    #ifdef DEBUG
    printf("%s\n", message);
    #endif
}
#endif // !UTILS
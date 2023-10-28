#include <math.h>
#include <stdint.h>

#define TEMPERATURENOMINAL 25
#define NTC_BETA 3650.0
#define NTC_NOMINAL_RESISTANCE 10000.0
#define NTC_SERIES_RESISTANCE 10000.0
#define NTC_ADC_MAX 4095.0
#define NTC_VCC 5.0
#define NTC_CONST_TEMP 298.15
#define NTC_CONV_TEMP 273.15
#define NTC_ANG_COEF 0.8757820472999968 
#define NTC_LINE_COEF 4.466406353995638 
#define NTC_OFFSET 0

float calculate_temp_raw(float voltage);

float calibrate_temp(float temp_raw);
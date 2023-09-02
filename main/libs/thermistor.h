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
#define NTC_ANG_COEF 0.9476300342686530 // 1.00199242
#define NTC_LINE_COEF 2.307249007566250 // 1.2487121159654997
#define NTC_OFFSET 0

float calculate_temp_raw(float voltage);

float calibrate_temp(float temp_raw);
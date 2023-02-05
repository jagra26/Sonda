#include <math.h>
#include <stdint.h>

#define TEMPERATURENOMINAL 25
#define NTC_BETA 3650.0
#define NTC_NOMINAL_RESISTANCE 10000.0
#define NTC_SERIES_RESISTANCE 10000.0
#define NTC_ADC_MAX 4095.0
#define NTC_VCC 3.3
#define NTC_CONST_TEMP 298.15
#define NTC_CONV_TEMP 273.15
#define NTC_ANG_COEF 1.00199242
#define NTC_LINE_COEF 1.2487121159654997
#define NTC_OFFSET 0

/**
 * @brief function to calculate temperature using the steinhart method
 * @todo create a library to thermistor
 *
 * @param adc_reading input value of adc
 * @return float
 */
float calculate_temp(uint32_t adc_reading);

float calculate_temp_2(uint32_t adc_reading);

float calculate_temp_3(uint32_t adc_reading);

float calculate_temp_4(float voltage);

float calibrate_temp(float temp_raw);
#include "thermistor.h"
#include <stdio.h>

float calculate_temp(uint32_t adc_reading) {
  float average;
  average = NTC_ADC_MAX / adc_reading - 1;
  average = NTC_SERIES_RESISTANCE * average;
  float steinhart;
  steinhart = average / NTC_NOMINAL_RESISTANCE;
  steinhart = log(steinhart);
  steinhart /= NTC_BETA;                                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + NTC_CONV_TEMP); // + (1/To)
  steinhart = 1.0 / steinhart;                             // Invert
  steinhart -= NTC_CONV_TEMP;                              // convert to C
  return steinhart;
}

float calculate_temp_2(uint32_t adc_reading) {
  float average;
  average = NTC_ADC_MAX / adc_reading - 1;
  average = NTC_SERIES_RESISTANCE * average;
  float steinhart;
  steinhart = average / NTC_NOMINAL_RESISTANCE;
  steinhart = log(steinhart);
  steinhart /= NTC_BETA;                                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + NTC_CONV_TEMP); // + (1/To)
  steinhart = 1.0 / steinhart;                             // Invert
  steinhart -= NTC_CONV_TEMP;                              // convert to C
  steinhart = 0.962 * steinhart + 3.384;
  return steinhart;
}

float calculate_temp_3(uint32_t adc_reading) {
  float average;
  average = NTC_ADC_MAX / adc_reading - 1;
  average = NTC_SERIES_RESISTANCE * average;
  float steinhart;
  steinhart = average / NTC_NOMINAL_RESISTANCE;
  steinhart = log(steinhart);
  steinhart /= NTC_BETA;                                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + NTC_CONV_TEMP); // + (1/To)
  steinhart = 1.0 / steinhart;                             // Invert
  steinhart -= NTC_CONV_TEMP;                              // convert to C
  steinhart = 1.00199242 * steinhart + 1.2487121159654997;
  return steinhart;
}

float calculate_temp_4(float voltage) {
  float R = ((NTC_VCC / voltage) - 1) * NTC_SERIES_RESISTANCE;
  printf("%f\n", R);
  float temp =
      1 / NTC_CONST_TEMP + (1 / NTC_BETA) * log(R / NTC_SERIES_RESISTANCE);
  printf("%f\n", temp);
  temp = 1 / temp;
  printf("%f\n", temp);
  temp -= NTC_CONV_TEMP;
  printf("%f\n", temp);
  return temp;
}

float calibrate_temp(float temp_raw) {
  return temp_raw * NTC_ANG_COEF + NTC_LINE_COEF + NTC_OFFSET;
}
#include "thermistor.h"

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
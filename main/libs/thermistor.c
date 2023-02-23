#include "thermistor.h"
#include <stdio.h>

float calculate_temp_raw(float voltage) {
  float R = ((NTC_VCC / voltage) - 1) * NTC_SERIES_RESISTANCE;
  float temp =
      1 / NTC_CONST_TEMP + (1 / NTC_BETA) * log(R / NTC_SERIES_RESISTANCE);
  temp = 1 / temp;
  temp -= NTC_CONV_TEMP;
  return temp;
}

float calibrate_temp(float temp_raw) {
  return temp_raw * NTC_ANG_COEF + NTC_LINE_COEF + NTC_OFFSET;
}
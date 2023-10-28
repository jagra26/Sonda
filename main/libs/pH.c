#include "pH.h"

float ph_calc(float voltage) {
  return 7 + (((VOLTAGE_7_0 - voltage) / a_m) + b);
}
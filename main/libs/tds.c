#include "tds.h"

static float temp_coef_calc(float temperature) {
  return LINEAR_COEF_TEMP_COEF +
         ANGULAR_COEF_TEMP_COEF * (temperature - TEMP_CONSTANT);
}

static float volt_comp_calc(float voltage, float temp_coef) {
  return voltage / temp_coef;
}

int tds_calc(float voltage, float temperature) {
  float ct = temp_coef_calc(temperature);
  float voltage_comp = volt_comp_calc(voltage, ct);
  float tds = CUBIC_COEF;
  tds *= voltage_comp;
  tds -= SQUARE_COEF;
  tds *= voltage_comp;
  tds += LINEAR_COEF;
  tds *= voltage_comp;
  tds /= 2;
  return (int)(tds) < 0 ? 0 : (int)(tds);
}
#include "turbidity.h"

float calculate_delta_u(float temp) { return -0.0192 * (temp - 25); }

float calculate_u_25(float voltage, float delta) { return voltage - delta; }

float calculate_K(float u_25) { return 865.68 * u_25; }

float calculate_turbidity(float voltage, float K) {
  return (-865.68 * voltage) + K;
}

float get_turbidity(float voltage) {
  return calculate_turbidity(voltage,
                             calculate_K(calculate_u_25(voltage, DELTA)));
}
#include "pH.h"

float ph_calc(float voltage) { return (voltage * ANGULAR_COEF) + LINEAR_COEF; }
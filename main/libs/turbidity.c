#include "turbidity.h"

int get_turbidity(float voltage) {
  if (voltage <= LEVEL_1_MAX) {
    return 1;
  } else if (voltage > LEVEL_1_MAX && voltage <= LEVEL_2_MAX) {
    return 2;
  } else if (voltage > LEVEL_2_MAX && voltage <= LEVEL_3_MAX) {
    return 3;
  } else {
    return 4;
  }
}
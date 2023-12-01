#include "turbidity.h"

int get_turbidity(float voltage) {
  if (voltage <= LEVEL_4_MAX) {
    return 4;
  } else if (voltage > LEVEL_4_MAX && voltage <= LEVEL_3_MAX) {
    return 3;
  } else if (voltage > LEVEL_3_MAX && voltage <= LEVEL_2_MAX) {
    return 2;
  } else {
    return 1;
  }
}
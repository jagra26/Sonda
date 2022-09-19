#include "adc.h"

float adc_multi_sampling(uint8_t pin, uint samples, uint ms) {
  float sample_sum = 0.0;
  for (uint8_t i = 0; i < samples; i++) {
    sample_sum += analogRead(pin);
    delay(ms);
  }
  return sample_sum / samples;
}

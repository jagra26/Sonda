#include "adc.h"

float digit_to_voltage(int16_t adc_read) {
  return adc_read * ADS1115_RESOLUTION / MILI_TO_VOLT;
}

void ads_config(ADS1115_lite *ads, int gain_param, int sample_rate) {
  ads->setGain(gain_param);
  ads->setSampleRate(sample_rate);
}

int16_t ads_read(ADS1115_lite *ads, int mux_param) {
  ads->setMux(mux_param);
  ads->triggerConversion();
  return ads->getConversion();
}
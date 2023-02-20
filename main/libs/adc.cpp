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

int16_t median_ads_read(ADS1115_lite *ads, int mux_param, int sample_number,
                        uint32_t delay_ms) {
  int16_t reads[sample_number] = {0};
  for (int i = 0; i < sample_number; i++) {
    reads[i] = ads_read(ads, mux_param);
    delay(delay_ms);
  }
  sort(reads, reads + sample_number);
  int half_index =
      (sample_number % 2) == 0 ? sample_number / 2 : (sample_number / 2) - 1;
  return reads[half_index];
}

int16_t mean_ads_read(ADS1115_lite *ads, int mux_param, int sample_number,
                      uint32_t delay_ms) {
  int16_t reads = 0;
  for (int i = 0; i < sample_number; i++) {
    reads += ads_read(ads, mux_param);
    delay(delay_ms);
  }
  return (int16_t)(reads / sample_number);
}
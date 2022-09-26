#include "adc.h"

void print_char_val_type(esp_adc_cal_value_t val_type) {
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    printf("Characterized using Two Point Value\n");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    printf("Characterized using eFuse Vref\n");
  } else {
    printf("Characterized using Default Vref\n");
  }
}

void check_efuse(void) {
  // Check if TP is burned into eFuse
  if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
    printf("eFuse Two Point: Supported\n");
  } else {
    printf("eFuse Two Point: NOT supported\n");
  }
  // Check Vref is burned into eFuse
  if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
    printf("eFuse Vref: Supported\n");
  } else {
    printf("eFuse Vref: NOT supported\n");
  }
}

void adc_init(void) {
  check_efuse();
  adc2_config_channel_atten((adc2_channel_t)channel, atten);
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type =
      esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
  print_char_val_type(val_type);
}

/**
 * @brief get samples of adc2 and return the average of them
 * @todo create library to adc usage
 *
 * @param channel
 * @param width
 * @param delay
 * @return float
 */
/*float multi_sampling_adc2(adc_channel_t channel, adc_bits_width_t width,
                          int delay) {
  uint32_t adc_reading = 0;
  // Multisampling
  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    int raw;
    adc2_get_raw((adc2_channel_t)channel, width, &raw);
    adc_reading += raw;
    vTaskDelay(pdMS_TO_TICKS(delay));
  }
  adc_reading /= NO_OF_SAMPLES;
  return adc_reading;
}*/

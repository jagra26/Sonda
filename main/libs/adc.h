//#include "driver/adc.h"
#include "esp_adc_cal.h"
//#include "freertos/task.h"
#include "Arduino.h"
#include <stdio.h>
#include <time.h>

#define DEFAULT_VREF 1100 // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 8   // Multisampling

static const adc_channel_t channel =
    ADC_CHANNEL_8; // 10 channels: GPIO0, GPIO2, GPIO4, GPIO12 - GPIO15, GPIO25
                   // - GPIO27
static const adc_bits_width_t width = ADC_WIDTH_BIT_12; // 12 bits de leitura
static const adc_atten_t atten = ADC_ATTEN_DB_11;       // 150 mV ~ 2450 mV
static const adc_unit_t unit = ADC_UNIT_2;

esp_adc_cal_characteristics_t *adc_chars;

void print_char_val_type(esp_adc_cal_value_t val_type);

void check_efuse(void);

void adc_init(void);

float multi_sampling_adc2(adc_channel_t channel, adc_bits_width_t width,
                          int delay);

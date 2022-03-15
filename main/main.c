#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lora.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const adc_channel_t channel =
    ADC_CHANNEL_6; // GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12; // 12 bits de leitura
static const adc_atten_t atten = ADC_ATTEN_DB_11;       // 150 mV ~ 2450 mV
static const adc_unit_t unit = ADC_UNIT_2;

#define DEFAULT_VREF 1100 // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 8 // Multisampling
static esp_adc_cal_characteristics_t *adc_chars;

#define TEMPERATURENOMINAL 25
#define NTC_BETA 3650.0
#define NTC_NOMINAL_RESISTANCE 10000.0
#define NTC_SERIES_RESISTANCE 10000.0
#define NTC_ADC_MAX 4095.0
#define NTC_VCC 3.3
#define NTC_CONST_TEMP 298.15
#define NTC_CONV_TEMP 273.15

// Task handles
static TaskHandle_t temp_task = NULL;
static TaskHandle_t tx_task = NULL;

// Queue handles
static QueueHandle_t temp_queue;

static void print_char_val_type(esp_adc_cal_value_t val_type) {
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    printf("Characterized using Two Point Value\n");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    printf("Characterized using eFuse Vref\n");
  } else {
    printf("Characterized using Default Vref\n");
  }
}

static void check_efuse(void) {
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

float calculate_temp(uint32_t adc_reading) {
  float average;
  average = NTC_ADC_MAX / adc_reading - 1;
  average = NTC_SERIES_RESISTANCE * average;
  float steinhart;
  steinhart = average / NTC_NOMINAL_RESISTANCE;
  steinhart = log(steinhart);
  steinhart /= NTC_BETA;                                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + NTC_CONV_TEMP); // + (1/To)
  steinhart = 1.0 / steinhart;                             // Invert
  steinhart -= NTC_CONV_TEMP;                              // convert to C
  printf("temp = %f\n", steinhart);
  return steinhart;
}

float multi_sampling_adc2(adc_channel_t channel, adc_bits_width_t width,
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
}

void task_tx(void *p) {
  char temp[10];
  while (true) {
    if (xQueueReceive(temp_queue, (void *)&temp, 10) == pdTRUE) {
      printf("pop: %s ºC\n", temp);
      lora_send_packet((uint8_t *)temp, strlen(temp));
      printf("packet sent...\n");
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void task_temp_read(void *p) {
  while (true) {
    float adc_reading = multi_sampling_adc2(channel, width, 10);
    float temp = calculate_temp(adc_reading);
    char temp_msg[10];
    sprintf(temp_msg, "%f", temp);
    xQueueSend(temp_queue, (void *)&temp_msg, 10);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void app_main() {
  temp_queue = xQueueCreate(10, sizeof(char[10]));
  while (temp_queue == NULL)
    ;
  check_efuse();
  adc2_config_channel_atten((adc2_channel_t)channel, atten);
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type =
      esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
  print_char_val_type(val_type);
  lora_init();
  lora_set_frequency(866e6);
  lora_enable_crc();
  xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, tx_task);
  xTaskCreate(&task_temp_read, "task_temp_read", 2048, NULL, 5, temp_task);
}

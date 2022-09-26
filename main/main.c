#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "libs/sd.h"
#include "lora.h"
#include "thermistor.h"
#include <esp_system.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

struct tm data; // Cria a estrutura que contem as informacoes da data.

static const char *LORATAG = "LORA";

// thermistor
static const char *THERMTAG = "THERMISTOR";
static const adc_channel_t channel =
    ADC_CHANNEL_8; // 10 channels: GPIO0, GPIO2, GPIO4, GPIO12 - GPIO15, GPIO25
                   // - GPIO27
static const adc_bits_width_t width = ADC_WIDTH_BIT_12; // 12 bits de leitura
static const adc_atten_t atten = ADC_ATTEN_DB_11;       // 150 mV ~ 2450 mV
static const adc_unit_t unit = ADC_UNIT_2;

#define DEFAULT_VREF 1100 // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 8 // Multisampling
static esp_adc_cal_characteristics_t *adc_chars;

// SPI pins of sd card
#define PIN_NUM_MISO 35
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK 14
#define PIN_NUM_CS 13

// Task handles
#define STACK_SIZE_SD 4096
StaticTask_t x_task_buffer_sd;
StackType_t x_stack_sd[STACK_SIZE_SD];
#define STACK_SIZE_TX 2048
StaticTask_t x_task_buffer_tx;
StackType_t x_stack_tx[STACK_SIZE_TX];
#define STACK_SIZE_TEMP 2048
StaticTask_t x_task_buffer_temp;
StackType_t x_stack_temp[STACK_SIZE_TEMP];
static TaskHandle_t temp_task = NULL;
static TaskHandle_t tx_task = NULL;
static TaskHandle_t sd_task = NULL;

// Queue handles
static QueueHandle_t temp_queue;
static QueueHandle_t tx_queue;

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

/**
 * @brief get samples of adc2 and return the average of them
 * @todo create library to adc usage
 *
 * @param channel
 * @param width
 * @param delay
 * @return float
 */
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

/**
 * @brief task to control LoRa transmission
 * @todo define String to transmit see JSON
 *
 * @param p
 */
void task_tx(void *p);

/**
 * @brief task to read temp
 *
 * @param p
 */
void task_temp_read(void *p);

/**
 * @brief task to save in sd card
 * @todo define String to transmit see JSON, use other sensors, semaphors
 *
 * @param p
 */
void task_sd(void *p);

void app_main() {
  // rtc
  setenv("TZ", "EST3EDT", 1);
  tzset();
  struct timeval tv;      // Cria a estrutura temporaria para funcao abaixo.
  tv.tv_sec = 1647825026; // Atribui minha data atual. Voce pode usar o NTP
                          // para
                          // isso ou o site citado no artigo!
  settimeofday(
      &tv, NULL); // Configura o RTC para manter a data atribuida atualizada.

  // mount_file_system(PIN_NUM_MOSI, PIN_NUM_MISO, PIN_NUM_CLK, PIN_NUM_CS);

  // create queues
  temp_queue = xQueueCreate(10, sizeof(char[10]));
  while (temp_queue == NULL)
    ;
  tx_queue = xQueueCreate(10, sizeof(char[128]));
  while (tx_queue == NULL)
    ;
  // adc thermistor
  check_efuse();
  adc2_config_channel_atten((adc2_channel_t)channel, atten);
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type =
      esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
  print_char_val_type(val_type);
  // lora
  lora_init();
  lora_set_frequency(866e6);
  lora_enable_crc();
  // tasks
  sd_task = xTaskCreateStatic(&task_sd, "task_sd", STACK_SIZE_SD, NULL, 10,
                              x_stack_sd, &x_task_buffer_sd);
  tx_task = xTaskCreateStatic(&task_tx, "task_tx", STACK_SIZE_TX, NULL, 5,
                              x_stack_tx, &x_task_buffer_tx);
  temp_task =
      xTaskCreateStatic(&task_temp_read, "task_temp_read", STACK_SIZE_TEMP,
                        NULL, 5, x_stack_temp, &x_task_buffer_temp);
}

void task_sd(void *p) {

  // Use POSIX and C standard library functions to work with files.

  // First create a file.
  mount_file_system(PIN_NUM_MOSI, PIN_NUM_MISO, PIN_NUM_CLK, PIN_NUM_CS);
  const char *data_file = MOUNT_POINT "/data.csv";
  ESP_LOGI(SDTAG, "Opening file %s", data_file);
  FILE *f;
  f = fopen(data_file, "w");
  if (f == NULL) {
    ESP_LOGE(SDTAG, "Failed to open file for writing");
    return;
  }
  ESP_LOGI(SDTAG, "write header");
  fprintf(f, "Data, Hora, Temperatura (ºC)\n");
  fclose(f);
  ESP_LOGI(SDTAG, "close file: %s", data_file);
  char temp_msg[10];
  char lora_msg[128] = "";
  time_t tt = time(NULL); // Obtem o tempo atual em segundos. Utilize isso
                          // sempre que precisar obter o tempo atual
  char data_formatada[64];
  while (true) {
    if (xQueueReceive(temp_queue, (void *)&temp_msg, 10) == pdTRUE) {
      ESP_LOGI(SDTAG, "Opening file %s", data_file);
      f = fopen(data_file, "a");
      if (f == NULL) {
        ESP_LOGE(SDTAG, "Failed to open file for writing");
        return;
      }
      data = *localtime(&tt); // Converte o tempo atual e atribui na estrutura

      strftime(data_formatada, 64, "%d/%m/%Y, %H:%M:%S",
               &data); // Cria uma String formatada da estrutura "data"
      ESP_LOGI(SDTAG, "get datetime: %s", data_formatada);
      fprintf(f, "%s, %s\n", data_formatada, temp_msg);
      ESP_LOGI(SDTAG, "write data on file");
      fclose(f);
      ESP_LOGI(SDTAG, "close file: %s", data_file);
      strcat(lora_msg, data_formatada);
      strcat(lora_msg, ", ");
      strcat(lora_msg, temp_msg);
      xQueueSend(tx_queue, (void *)&lora_msg, 10);
      ESP_LOGI(SDTAG, "write on tx_queue");
      strcpy(lora_msg, "");
      ESP_LOGI(SDTAG, "reset lora_msg");
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void task_temp_read(void *p) {
  while (true) {
    float adc_reading = multi_sampling_adc2(channel, width, 10);
    float temp = calculate_temp(adc_reading);
    char temp_msg[10];
    sprintf(temp_msg, "%.2f", temp);
    xQueueSend(temp_queue, (void *)&temp_msg, 10);
    ESP_LOGI(THERMTAG, "write on temp_queue");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void task_tx(void *p) {
  char payload[128];
  while (true) {
    if (xQueueReceive(tx_queue, (void *)&payload, 10) == pdTRUE) {
      ESP_LOGI(LORATAG, "Lenght of payload: %d", strlen(payload));
      lora_send_packet((uint8_t *)payload, strlen(payload));
      ESP_LOGI(LORATAG, "Packet send: %s", payload);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

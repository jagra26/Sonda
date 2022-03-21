#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lora.h"
#include "sdmmc_cmd.h"
#include <esp_system.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <time.h>

struct tm data; // Cria a estrutura que contem as informacoes da data.

static const char *LORATAG = "LORA";

// thermistor
static const char *THERMTAG = "THERMISTOR";
static const adc_channel_t channel =
    ADC_CHANNEL_8; // 10 channels: GPIO0, GPIO2, GPIO4, GPIO12 - GPIO15, GOIO25
                   // - GPIO27
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

// sd card
static const char *SDTAG = "SD CARD";
#define SD true
#define MOUNT_POINT "/sdcard"
#define SDSPI_DEFAULT_DMA 1

// Pin assignments can be set in menuconfig, see "SD SPI Example Configuration"
// menu. You can also change the pin assignments here by changing the following
// 4 lines.
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK 14
#define PIN_NUM_CS 13

// Task handles
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
 * @brief function to calculate temperature using the steinhart method
 * @todo create a library to thermistor
 *
 * @param adc_reading input value of adc
 * @return float
 */
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
  return steinhart;
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

/**
 * @brief task to read temp
 *
 * @param p
 */
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

/**
 * @brief task to save in sd card
 * @todo define String to transmit see JSON, use other sensors, semaphors
 *
 * @param p
 */
void task_sd(void *p) {

  // Use POSIX and C standard library functions to work with files.

  // First create a file.
  const char *data_file = MOUNT_POINT "/data.csv";
  ESP_LOGI(SDTAG, "Opening file %s", data_file);
  FILE *f = fopen(data_file, "w");
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
  while (true) {
    if (xQueueReceive(temp_queue, (void *)&temp_msg, 10) == pdTRUE) {
      ESP_LOGI(SDTAG, "Opening file %s", data_file);
      FILE *f = fopen(data_file, "a");
      if (f == NULL) {
        ESP_LOGE(SDTAG, "Failed to open file for writing");
        return;
      }
      time_t tt = time(NULL); // Obtem o tempo atual em segundos. Utilize isso
                              // sempre que precisar obter o tempo atual
      data = *gmtime(&tt);    // Converte o tempo atual e atribui na estrutura

      char data_formatada[64];
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

void app_main() {
  // rtc
  struct timeval tv;      // Cria a estrutura temporaria para funcao abaixo.
  tv.tv_sec = 1647825026; // Atribui minha data atual. Voce pode usar o NTP
                          // para
                          // isso ou o site citado no artigo!
  settimeofday(
      &tv, NULL); // Configura o RTC para manter a data atribuida atualizada.
  if (SD) {
    // sd card
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(SDTAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT
    // filesystem. Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience
    // functions. Please check its source code and implement error recovery when
    // developing production applications.
    ESP_LOGI(SDTAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
      ESP_LOGE(SDTAG, "Failed to initialize bus.");
      return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP)
    // signals. Modify slot_config.gpio_cd and slot_config.gpio_wp if your board
    // has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(SDTAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config,
                                  &mount_config, &card);

    if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
        ESP_LOGE(SDTAG,
                 "Failed to mount filesystem. "
                 "If you want the card to be formatted, set the "
                 "CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
      } else {
        ESP_LOGE(SDTAG,
                 "Failed to initialize the card (%s). "
                 "Make sure SD card lines have pull-up resistors in place.",
                 esp_err_to_name(ret));
      }
      return;
    }
    ESP_LOGI(SDTAG, "Filesystem mounted");
    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
  }

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
  xTaskCreate(&task_sd, "task_sd", 2048, NULL, 10, sd_task);
  xTaskCreate(&task_tx, "task_tx", 2048, NULL, 5, tx_task);
  xTaskCreate(&task_temp_read, "task_temp_read", 2048, NULL, 5, temp_task);
}

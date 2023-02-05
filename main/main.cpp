
#include "ADS1115_lite.h"
#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SD_utils.h"
#include "SPI.h"
#include "TinyGPSPlus.h"
#include "Wire.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

extern "C" {
#include "adc.h"
#include "lora.h"
#include "thermistor.h"
}
#include <esp_system.h>
#include <stdio.h>
#include <time.h>

#define SCK 14
#define MISO 35
#define MOSI 15
#define CS 13
#define SDA_1 25
#define SCL_1 33

ADS1115_lite ads(ADS1115_DEFAULT_ADDRESS);

TinyGPSPlus _gps;
HardwareSerial _serial_gps(1);
struct tm data; // Cria a estrutura que contem as informacoes da data.

static const char *LORATAG = "LORA";

// thermistor
static const char *THERMTAG = "THERMISTOR";

// Task handles
#define STACK_SIZE_SD 4096
StaticTask_t x_task_buffer_sd;
StackType_t x_stack_sd[STACK_SIZE_SD];
#define STACK_SIZE_TX 4096
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

SPIClass spi = SPIClass(VSPI);

void ads_config() {
  ads.setGain(ADS1115_REG_CONFIG_PGA_6_144V);
  ads.setSampleRate(ADS1115_REG_CONFIG_DR_128SPS);
}

int16_t ads_read() {
  ads.setMux(ADS1115_REG_CONFIG_MUX_SINGLE_0); // Single mode input on AIN0
                                               // (AIN0 - GND)
  ads.triggerConversion();                     // Triggered mannually
  return ads.getConversion();                  // returns int16_t value
}

extern "C" void app_main() {
  Serial.begin(115200);
  delay(10);
  ads_config();

  _serial_gps.begin(9600, SERIAL_8N1, 34, 12);
  while (_serial_gps.available()) {
    _gps.encode(_serial_gps.read());
  }
  setenv("TZ", "EST3EDT", 1);
  tzset();
  struct timeval tv;      // Cria a estrutura temporaria para funcao abaixo.
  tv.tv_sec = 1647825026; // Atribui minha data atual. Voce pode usar o NTP
                          // para
                          // isso ou o site citado no artigo!
  settimeofday(
      &tv, NULL); // Configura o RTC para manter a data atribuida atualizada.

  // create queues
  temp_queue = xQueueCreate(10, sizeof(char[10]));
  while (temp_queue == NULL)
    ;
  tx_queue = xQueueCreate(10, sizeof(char[128]));
  while (tx_queue == NULL)
    ;

  // ESP_LOGI(SDTAG, "SD task Start");
  spi.begin(SCK, MISO, MOSI, CS);
  ESP_LOGI(SDTAG, "SPI begin");
  vTaskDelay(pdMS_TO_TICKS(500));
  if (!SD.begin(CS, spi, 80000000)) {
    ESP_LOGE(SDTAG, "Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  checkSD(cardType);
  //

  // tasks
  xTaskCreate(task_tx, "task_tx", STACK_SIZE_TX, NULL, 5, &tx_task);
  xTaskCreate(task_temp_read, "task_temp_read", STACK_SIZE_TEMP, NULL, 5,
              &temp_task);
  xTaskCreate(task_sd, "task_sd", STACK_SIZE_SD, NULL, 10, &sd_task);
  vTaskDelete(NULL);
}

void task_sd(void *p) {

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  ESP_LOGI(SDTAG, "SD Card Size: %lluMB\n", cardSize);
  writeFile(SD, "/data.csv", "Data, Hora, Temperatura (ºC)\n");

  char temp_msg[10];
  char lora_msg[128] = "";
  char line[128] = "";
  time_t tt = time(NULL); // Obtem o tempo atual em segundos. Utilize isso
                          // sempre que precisar obter o tempo atual
  char data_formatada[64];
  while (true) {
    if (xQueueReceive(temp_queue, (void *)&temp_msg, 10) == pdTRUE) {
      ESP_LOGI(SDTAG, "%s", temp_msg);
      data = *localtime(&tt); // Converte o tempo atual e atribui na estrutura

      strftime(data_formatada, 64, "%d/%m/%Y, %H:%M:%S",
               &data); // Cria uma String formatada da estrutura "data"
      ESP_LOGI(SDTAG, "get datetime: %s", data_formatada);
      sprintf(line, "%s, %s\n", data_formatada, temp_msg);
      appendFile(SD, "/data.csv", line);
      sprintf(lora_msg, "%s, %s", data_formatada, temp_msg);
      xQueueSend(tx_queue, (void *)&lora_msg, 10);
      ESP_LOGI(SDTAG, "write on tx_queue");
      strcpy(lora_msg, "");
      ESP_LOGI(SDTAG, "reset lora_msg");
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void task_temp_read(void *p) {
  // adc_init();
  // Serial.begin(9600);
  int16_t adc0;
  float Voltage = 0.0;

  while (true) {
    // float adc_reading = multi_sampling_adc2(channel, width, 10);
    adc0 = ads_read();
    ESP_LOGI(THERMTAG, "%d", adc0);
    adc0 <= 0 ? adc0 = 0 : adc0 = adc0;
    Voltage = digit_to_voltage(adc0);
    ESP_LOGI(THERMTAG, "%f", Voltage);
    float temp = calculate_temp_4(Voltage);
    ESP_LOGI(THERMTAG, "%f", temp);
    temp = calibrate_temp(temp);
    // float temp = calculate_temp_3(adc_reading);
    char temp_msg[10];
    sprintf(temp_msg, "%.2f", temp);
    ESP_LOGI(THERMTAG, "%s", temp_msg);
    xQueueSend(temp_queue, (void *)&temp_msg, 10);
    ESP_LOGI(THERMTAG, "write on temp_queue");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void task_tx(void *p) {
  vTaskDelay(pdMS_TO_TICKS(1000));
  lora_init();
  ESP_LOGI(LORATAG, "lora initialized");
  lora_set_frequency(866e6);
  ESP_LOGI(LORATAG, "lora frequency seted");
  lora_enable_crc();
  ESP_LOGI(LORATAG, "lora enable crc");
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
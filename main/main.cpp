#include "ADS1115_lite.h"
#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SD_utils.h"
#include "SPI.h"
#include "TinyGPSPlus.h"
#include "Wire.h"
#include "adc.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

extern "C" {
#include "lora.h"
#include "tds.h"
#include "thermistor.h"
}
#include "time.h"
#include <WiFi.h>
#include <esp_system.h>
#include <stdio.h>

#define SCK 14
#define MISO 35
#define MOSI 15
#define CS 13

const char *ssid = "brisa-1921072";
const char *password = "to7w55gc";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3;
const int daylightOffset_sec = 3600;

ADS1115_lite ads(ADS1115_DEFAULT_ADDRESS);
SPIClass spi = SPIClass(VSPI);

TinyGPSPlus _gps;
HardwareSerial _serial_gps(1);
struct tm data; // Cria a estrutura que contem as informacoes da data.

// LoRa Log
static const char *LORATAG = "LORA";

// Sensor Log
static const char *SENSORTAG = "SENSOR";

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
static QueueHandle_t sensor_queue;
static QueueHandle_t tx_queue;

#define SENSOR_MSG_SIZE 20
#define TX_MSG_SIZE 256

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
void task_sensor_read(void *p);

/**
 * @brief task to save in sd card
 * @todo define String to transmit see JSON, use other sensors, semaphors
 *
 * @param p
 */
void task_sd(void *p);

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

extern "C" void app_main() {
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  delay(10);
  ads_config(&ads, ADS1115_REG_CONFIG_PGA_6_144V, ADS1115_REG_CONFIG_DR_128SPS);
  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  String mcz_tz = "<-03>3";
  setenv("TZ", mcz_tz.c_str(), 1);
  tzset();
  printLocalTime();
  struct tm timeinfo;
  struct timeval tv;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.print("trying to get datetime ");
  }
  tv.tv_sec = mktime(&timeinfo);
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);

  // disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  /*_serial_gps.begin(9600, SERIAL_8N1, 34, 12);
  while (_serial_gps.available()) {
    _gps.encode(_serial_gps.read());
  }*/

  // create queues
  sensor_queue = xQueueCreate(10, sizeof(char[SENSOR_MSG_SIZE]));
  while (sensor_queue == NULL)
    ;
  tx_queue = xQueueCreate(10, sizeof(char[TX_MSG_SIZE]));
  while (tx_queue == NULL)
    ;

  spi.begin(SCK, MISO, MOSI, CS);
  ESP_LOGI(SDTAG, "SPI begin");
  vTaskDelay(pdMS_TO_TICKS(500));
  while (!SD.begin(CS, spi, 80000000, "/sd", 10)) {
    ESP_LOGE(SDTAG, "Card Mount Failed - retrying in 5s");
    delay(500);
  }
  uint8_t cardType = SD.cardType();

  checkSD(cardType);

  // tasks
  xTaskCreate(task_tx, "task_tx", STACK_SIZE_TX, NULL, 5, &tx_task);
  xTaskCreate(task_sensor_read, "task_sensor_read", STACK_SIZE_TEMP, NULL, 5,
              &temp_task);
  xTaskCreate(task_sd, "task_sd", STACK_SIZE_SD, NULL, 10, &sd_task);
  vTaskDelete(NULL);
}

void task_sensor_read(void *p) {
  int16_t adc0;
  int16_t adc1;
  float Voltage = 0.0;
  float temp;
  int tds;
  char sensor_msg[SENSOR_MSG_SIZE];

  while (true) {
    digitalWrite(2, HIGH);
    adc0 = median_ads_read(&ads, ADS1115_REG_CONFIG_MUX_SINGLE_0, 32, 40);
    adc0 <= 0 ? adc0 = 0 : adc0 = adc0;
    Voltage = digit_to_voltage(adc0);
    temp = calculate_temp_raw(Voltage);
    temp = calibrate_temp(temp);
    ESP_LOGI(SENSORTAG, "Temperature: %.2fºC", temp);
    adc1 = median_ads_read(&ads, ADS1115_REG_CONFIG_MUX_SINGLE_1, 32, 40);
    Voltage = digit_to_voltage(adc1);
    tds = tds_calc(Voltage, temp);
    ESP_LOGI(SENSORTAG, "TDS: %d ppm", tds);
    sprintf(sensor_msg, "%.2f, %d", temp, tds);
    ESP_LOGI(SENSORTAG, "%s", sensor_msg);
    xQueueSend(sensor_queue, (void *)&sensor_msg, 10);
    ESP_LOGI(SENSORTAG, "Write on sensor_queue");
    digitalWrite(2, LOW);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void task_sd(void *p) {

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  ESP_LOGI(SDTAG, "SD Card Size: %lluMB\n", cardSize);
  writeFile(SD, "/d.csv", "Data, Hora, Temperatura (ºC), TDS (ppm)\n");
  appendFile(SD, "/d.csv", "-, -, -, -, -\n");

  char sensor_msg[SENSOR_MSG_SIZE];
  char lora_msg[TX_MSG_SIZE] = "";
  char line[TX_MSG_SIZE] = "";
  time_t tt = time(NULL); // Obtem o tempo atual em segundos. Utilize isso
                          // sempre que precisar obter o tempo atual
  char data_formatada[64];
  while (true) {
    tt = time(NULL);
    if (xQueueReceive(sensor_queue, (void *)&sensor_msg, 10) == pdTRUE) {
      ESP_LOGI(SDTAG, "%s", sensor_msg);
      data = *localtime(&tt); // Converte o tempo atual e atribui na estrutura

      strftime(data_formatada, 64, "%d/%m/%Y, %H:%M:%S",
               &data); // Cria uma String formatada da estrutura "data"
      ESP_LOGI(SDTAG, "get datetime: %s", data_formatada);
      sprintf(line, "%s, %s\n", data_formatada, sensor_msg);
      ESP_LOGI(SDTAG, "line: %s", line);
      appendFile(SD, "/d.csv", line);
      sprintf(lora_msg, "%s, %s", data_formatada, sensor_msg);
      xQueueSend(tx_queue, (void *)&lora_msg, 10);
      ESP_LOGI(SDTAG, "write on tx_queue");
      strcpy(lora_msg, "");
      ESP_LOGI(SDTAG, "reset lora_msg: %s", lora_msg);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
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
  char payload[TX_MSG_SIZE] = "";
  while (true) {
    if (xQueueReceive(tx_queue, (void *)&payload, 10) == pdTRUE) {
      ESP_LOGI(LORATAG, "Lenght of payload: %d", strlen(payload));
      lora_send_packet((uint8_t *)payload, strlen(payload));
      ESP_LOGI(LORATAG, "Packet send: %s", payload);
      strcpy(payload, "");
      // lora_reset();
    }
    // lora_sleep();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
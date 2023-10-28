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
#include "time.h"
#include <WiFi.h>
#include <Wire.h>
#include <axp20x.h>
#include <esp_system.h>
#include <stdio.h>

extern "C" {
#include "lora.h"
#include "pH.h"
#include "tds.h"
#include "thermistor.h"
#include "turbidity.h"
}

#define SCK 14
#define MISO 35
#define MOSI 15
#define CS 13
#define RELAY 2

static const char *GPSTAG = "GPS";
static const char *MAINTAG = "MAIN";

const char *ssid = "brisa-1921072";
const char *password = "to7w55gc";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3;
const int daylightOffset_sec = 3600;

ADS1115_lite ads(ADS1115_DEFAULT_ADDRESS);
SPIClass spi = SPIClass(HSPI);

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
#define STACK_SIZE_SENSOR 2048
StaticTask_t x_task_buffer_sensor;
StackType_t x_stack_sensor[STACK_SIZE_SENSOR];
#define STACK_SIZE_GPS 2048
StaticTask_t x_task_buffer_gps;
StackType_t x_stack_gps[STACK_SIZE_GPS];
static TaskHandle_t sensor_task = NULL;
static TaskHandle_t tx_task = NULL;
static TaskHandle_t sd_task = NULL;
static TaskHandle_t gps_task = NULL;
#define SENSOR_PRIORITY 5
#define SD_PRIORITY 10
#define TX_PRIORITY 5
#define GPS_PRIORITY 1

// Queue handles
static QueueHandle_t sensor_queue;
uint8_t sensor_queue_size = 10;
static QueueHandle_t tx_queue;
uint8_t tx_queue_size = 10;

#define SENSOR_MSG_SIZE 24
#define TX_MSG_SIZE 256

#define HALF_SECOND 500
#define ONE_SECOND HALF_SECOND * 2
#define ONE_MINUTE ONE_SECOND * 60
#define ONE_HOUR ONE_MINUTE * 60
#define SERIAL_SPEED 115200
#define SPI_FREQUENCY 80000000
#define NUMBER_OF_FILES 10
#define EMPTY_STRING ""
#define LORA_FREQUENCY 866e6

struct tm timeinfo;
struct timeval tv;

void task_tx(void *p);

void task_sensor_read(void *p);

void task_sd(void *p);

void task_gps(void *p);

extern "C" void app_main() {
  pinMode(RELAY, OUTPUT);

  Serial.begin(SERIAL_SPEED);
  delay(HALF_SECOND);

  ads_config(&ads, ADS1115_REG_CONFIG_PGA_6_144V, ADS1115_REG_CONFIG_DR_128SPS);

  // Connect to Wi-Fi
  ESP_LOGI(MAINTAG, "Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(HALF_SECOND);
    ESP_LOGI(MAINTAG, ".");
  }

  ESP_LOGI(MAINTAG, "\nWiFi connected.\n");

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  /*String mcz_tz = "GMTGMT-1,M3.4.0/01,M10.4.0/02";
  setenv("TZ", mcz_tz.c_str(), 1);
  tzset();*/
  while (!getLocalTime(&timeinfo)) {
    delay(HALF_SECOND);
    Serial.print("trying to get datetime ");
  }
  tv.tv_sec = mktime(&timeinfo);
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);

  // disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  _serial_gps.begin(9600, SERIAL_8N1, 34, 12);

  // create queues
  sensor_queue = xQueueCreate(sensor_queue_size, sizeof(char[SENSOR_MSG_SIZE]));
  while (sensor_queue == NULL)
    ;
  tx_queue = xQueueCreate(tx_queue_size, sizeof(char[TX_MSG_SIZE]));
  while (tx_queue == NULL)
    ;

  spi.begin(SCK, MISO, MOSI, CS);
  ESP_LOGI(SDTAG, "SPI begin");
  delay(HALF_SECOND);
  while (!SD.begin(CS, spi, SPI_FREQUENCY, "/sd", NUMBER_OF_FILES)) {
    ESP_LOGE(SDTAG, "Card Mount Failed - retrying in 5s");
    delay(HALF_SECOND);
  }
  uint8_t cardType = SD.cardType();

  checkSD(cardType);

  lora_init();
  ESP_LOGI(LORATAG, "lora initialized");
  delay(ONE_SECOND);
  lora_set_frequency(LORA_FREQUENCY);
  ESP_LOGI(LORATAG, "lora frequency seted");
  lora_enable_crc();
  ESP_LOGI(LORATAG, "lora enable crc");

  // tasks
  xTaskCreate(task_tx, "task_tx", STACK_SIZE_TX, NULL, TX_PRIORITY, &tx_task);
  xTaskCreate(task_sensor_read, "task_sensor_read", STACK_SIZE_SENSOR, NULL,
              SENSOR_PRIORITY, &sensor_task);
  xTaskCreate(task_sd, "task_sd", STACK_SIZE_SD, NULL, SD_PRIORITY, &sd_task);
  xTaskCreate(task_gps, "task_gps", STACK_SIZE_GPS, NULL, GPS_PRIORITY,
              &gps_task);
  vTaskDelete(NULL);
}

/**
 * @brief task to read sensor values
 *
 * @param p
 */
void task_sensor_read(void *p) {
  int16_t adc0, adc1, adc2, adc3;
  float Voltage = 0.0;
  float temp, ph;
  int tds, turbidity;
  char sensor_msg[SENSOR_MSG_SIZE];

  while (true) {
    // Turn on sensors
    digitalWrite(RELAY, HIGH);
    // vTaskDelay(pdMS_TO_TICKS(ONE_SECOND * 60));

    // read temperature
    adc0 = median_ads_read(&ads, ADS1115_REG_CONFIG_MUX_SINGLE_0, 32, 40);
    adc0 <= 0 ? adc0 = 0 : adc0 = adc0;
    Voltage = digit_to_voltage(adc0);
    temp = calculate_temp_raw(Voltage);
    ESP_LOGI(SENSORTAG, "Temperature raw: %.1fºC", temp);
    temp = calibrate_temp(temp);
    ESP_LOGI(SENSORTAG, "Temperature: %.1fºC", temp);

    // read TDS
    adc1 = median_ads_read(&ads, ADS1115_REG_CONFIG_MUX_SINGLE_1, 32, 40);
    Voltage = digit_to_voltage(adc1);
    tds = tds_calc(Voltage, temp);
    tds = tds_calib(tds);
    ESP_LOGI(SENSORTAG, "TDS: %d ppm", tds);

    // read turbidity
    adc2 = median_ads_read(&ads, ADS1115_REG_CONFIG_MUX_SINGLE_2, 32, 40);
    Voltage = digit_to_voltage(adc2);
    turbidity = get_turbidity(Voltage);

    ESP_LOGI(SENSORTAG, "Turbidity: %d", turbidity);

    // read pH
    adc3 = median_ads_read(&ads, ADS1115_REG_CONFIG_MUX_SINGLE_3, 32, 40);
    Voltage = digit_to_voltage(adc3);
    ph = Voltage;
    ESP_LOGI(SENSORTAG, "ph voltage: %.1f", ph);
    ph = ph_calc(ph);
    ESP_LOGI(SENSORTAG, "ph: %.2f", ph);

    sprintf(sensor_msg, "%.1f, %d, %d, %.1f", temp, tds, turbidity, ph);
    ESP_LOGI(SENSORTAG, "%s", sensor_msg);
    xQueueSend(sensor_queue, (void *)&sensor_msg, 10);
    ESP_LOGI(SENSORTAG, "Write on sensor_queue");
    digitalWrite(RELAY, LOW);
    vTaskDelay(pdMS_TO_TICKS(ONE_SECOND * 2));
  }
}

/**
 * @brief task to save in sd card
 * @todo define String to transmit see JSON, use other sensors, semaphors
 *
 * @param p
 */
void task_sd(void *p) {

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  ESP_LOGI(SDTAG, "SD Card Size: %lluMB\n", cardSize);
  if (!checkFile(SD, "/d.csv")) {
    writeFile(SD, "/d.csv",
              "Date, Time, Temperature (ºC), TDS (ppm), Turbidity(V), pH\n");
    appendFile(SD, "/d.csv", "-, -, -, -, -, -\n");
  }

  char sensor_msg[SENSOR_MSG_SIZE];
  char lora_msg[TX_MSG_SIZE] = EMPTY_STRING;
  char line[TX_MSG_SIZE] = EMPTY_STRING;
  time_t tt = time(NULL);
  char date[64];
  while (true) {
    tt = time(NULL);
    if (xQueueReceive(sensor_queue, (void *)&sensor_msg, 10) == pdTRUE) {
      ESP_LOGI(SDTAG, "%s", sensor_msg);
      data = *localtime(&tt); // Converte o tempo atual e atribui na estrutura

      strftime(date, 64, "%d/%m/%Y, %H:%M:%S",
               &data); // Cria uma String formatada da estrutura "data"
      ESP_LOGI(SDTAG, "get datetime: %s", date);
      sprintf(line, "%s, %s\n", date, sensor_msg);
      ESP_LOGI(SDTAG, "line: %s", line);
      appendFile(SD, "/d.csv", line);
      sprintf(lora_msg, "%s, %s", date, sensor_msg);
      xQueueSend(tx_queue, (void *)&lora_msg, 10);
      ESP_LOGI(SDTAG, "write on tx_queue");
      strcpy(lora_msg, EMPTY_STRING);
      ESP_LOGI(SDTAG, "reset lora_msg: %s", lora_msg);
      vTaskDelay(pdMS_TO_TICKS(ONE_SECOND));
    }
  }
}

/**
 * @brief task to control LoRa transmission
 * @todo define String to transmit see JSON
 *
 * @param p
 */
void task_tx(void *p) {
  vTaskDelay(pdMS_TO_TICKS(ONE_SECOND));
  char payload[TX_MSG_SIZE] = EMPTY_STRING;
  while (true) {
    if (xQueueReceive(tx_queue, (void *)&payload, 10) == pdTRUE) {
      vTaskDelay(pdMS_TO_TICKS(ONE_SECOND));
      lora_send_packet((uint8_t *)payload, strlen(payload));
      ESP_LOGI(LORATAG, "Packet send: %s", payload);
      strcpy(payload, EMPTY_STRING);
      lora_sleep();
    }
    vTaskDelay(pdMS_TO_TICKS(ONE_SECOND));
  }
}

void task_gps(void *p) {
  unsigned long last_time = millis();
  while (_serial_gps.available()) {
    _gps.encode(_serial_gps.read());
    if (millis() - last_time >= ONE_MINUTE * 10) {
      last_time = millis();
      vTaskDelay(ONE_MINUTE * 10);
    }
    if (_gps.location.isValid()) {
      ESP_LOGI(GPSTAG, "gps values:\n Latitude: %10.6f\nLatitude: %10.6f\n",
               _gps.location.lat(), _gps.location.lng());
    }
    if (_gps.date.year() >= timeinfo.tm_year + 1900) {
      Serial.print(_gps.date.month());
      Serial.print(F("/"));
      Serial.print(_gps.date.day());
      Serial.print(F("/"));
      Serial.println(_gps.date.year());
      Serial.print(_gps.time.hour());
      Serial.print(":");
      Serial.print(_gps.time.minute());
      Serial.print(":");
      Serial.println(_gps.time.second());

      timeinfo.tm_year = _gps.date.year() - 1900;
      timeinfo.tm_mon = _gps.date.month() - 1; // Month, 0 - jan
      timeinfo.tm_mday = _gps.date.day();      // Day of the month
      timeinfo.tm_hour = _gps.time.hour();
      timeinfo.tm_min = _gps.time.minute();
      timeinfo.tm_sec = _gps.time.second();
      tv.tv_sec = mktime(&timeinfo);
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
      String mcz_tz = "<-03>3";
      setenv("TZ", mcz_tz.c_str(), 1);
      tzset();
    }
    vTaskDelay(ONE_SECOND);
  }
}
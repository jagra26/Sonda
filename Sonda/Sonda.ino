
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sd_card_utils.h"
#include "thermistor.h"

#define SCK 14
#define MISO 35
#define MOSI 15
#define CS 13
#define MOUNT_POINT "/sd"
#define THERMISTOR 25
#define NO_OF_SAMPLES 8
#define DELAY_ADC 10

#define STACK_SIZE_SD 4096
StaticTask_t x_task_buffer_sd;
StackType_t x_stack_sd[STACK_SIZE_SD];
static TaskHandle_t sd_task = NULL;

#define STACK_SIZE_TEMP 2048
StaticTask_t x_task_buffer_temp;
StackType_t x_stack_temp[STACK_SIZE_TEMP];
static TaskHandle_t temp_task = NULL;

void task_sd(void *p);
void task_temp(void *p);

// TODO: create temp task
// TODO: create queues
// TODO: create lora task

void setup() {
  Serial.begin(115200);
  pinMode(THERMISTOR, INPUT);

  sd_task = xTaskCreateStatic(&task_sd, "task_sd", STACK_SIZE_SD, NULL, 10,
                              x_stack_sd, &x_task_buffer_sd);
  temp_task =
      xTaskCreateStatic(&task_temp, "task_temp", STACK_SIZE_TEMP,
                        NULL, 5, x_stack_temp, &x_task_buffer_temp);
}

void loop() {}

void task_sd(void *p) {
  // Initialize with log level and log output.
  SPIClass spi = SPIClass(VSPI);
  spi.begin(SCK, MISO, MOSI, CS);
  if (!SD.begin(CS, spi, 80000000, MOUNT_POINT)) {
    Serial.println("Card Mount Failed");
    return;
  }
  
  if(!check_cardType(SD.cardType())) {
    return;
  }
  get_cardSize_MB(SD.cardSize());

  writeFile(SD, "/data.csv", "Epoch Time, Temperature(ºC)\r\n");
  uint32_t raw;
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void task_temp(void *p){
  uint32_t raw;
  while(true) {
    raw = (uint32_t)adc_multi_sampling(THERMISTOR, NO_OF_SAMPLES, DELAY_ADC);
    Serial.println(calculate_temp(raw));
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
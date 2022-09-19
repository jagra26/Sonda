/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-microsd-card-arduino/
  
  This sketch was mofidied from: Examples > SD(esp32) > SD_Test
*/

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "sd_card_utils.h"


#define SCK  14
#define MISO  35
#define MOSI  15
#define CS  13
#define THERMISTOR 25

#define STACK_SIZE_SD 4096
StaticTask_t x_task_buffer_sd;
StackType_t x_stack_sd[STACK_SIZE_SD];
static TaskHandle_t sd_task = NULL;

void setup(){
  Serial.begin(115200);
  // Initialize with log level and log output. 
  SPIClass spi = SPIClass(VSPI);
  spi.begin(SCK, MISO, MOSI, CS);

  if (!SD.begin(CS,spi,80000000)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);


  sd_task = xTaskCreateStatic(&task_sd, "task_sd", STACK_SIZE_SD, NULL, 10,
                              x_stack_sd, &x_task_buffer_sd);
  
  
}

void loop(){

}
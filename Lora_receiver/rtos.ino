#include "LoRa.h"
#include <Arduino.h>
#include <FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// define the pins used by the transceiver module
#define ss 18
#define rst 14
#define dio0 26

void task_rx(void *p) {
  for (;;) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      // received a packet
      Serial.print("Received packet '");

      // read packet
      while (LoRa.available()) {
        Serial.print((char)LoRa.read());
      }

      // print RSSI of packet
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
    }
    LoRa.read();
    vTaskDelay(1);
  }
}
void setup() {
  // setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  Serial.begin(9600);

  // replace the LoRa.begin(---E-) argument with your location's frequency
  // 433E6 for Asia
  // 866E6 for Europe
  // 915E6 for North America
  while (!LoRa.begin(866E6)) {
    Serial.println(".");
    delay(500);
  }
  xTaskCreate(&task_rx, "task_rx", 2048, NULL, 5, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
}
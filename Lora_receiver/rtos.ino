#include "LoRa.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <string.h>

// define the pins used by the transceiver module
#define ss 18
#define rst 14
#define dio0 26

#define LORA_MSG_SIZE 256
static QueueHandle_t lora_queue;

const char* ssid     = "brisa-1921072";
const char* password = "to7w55gc";

// ThingSpeak information
char thingSpeakAddress[] = "api.thingspeak.com";
unsigned long channelID = 1998424;
char* readAPIKey = "OL7XLXZBSTHO55I6";
char* writeAPIKey = "849B5ZPP512LA7VS";
const unsigned long postingInterval = 120L * 1000;

unsigned long lastConnectionTime = 0;
long lastUpdateTime = 0;
WiFiClient wifiClient;


void task_rx(void *p);
void task_wifi(void *p);

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
  lora_queue = xQueueCreate(10, sizeof(char[LORA_MSG_SIZE]));
  xTaskCreate(&task_rx, "task_rx", 2048, NULL, 5, NULL);
  xTaskCreate(&task_wifi, "task_wifi", 2048, NULL, 5, NULL);
}

void loop() {}

void task_rx(void *p) {
  String lora_packet;
  while(true) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      // received a packet
      Serial.print("Received packet '");

      // read packet
      while (LoRa.available()) {
        lora_packet = LoRa.readString();
        Serial.print(lora_packet);
        xQueueSend(lora_queue, (void *)&lora_packet, 10);
      }

      // print RSSI of packet
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
    }
    LoRa.read();
    vTaskDelay(1);
  }
}

void payload_separate(String lora_msg, char *date, char *time, char *temp, char*TDS, char *turbidity, char *ph){
        char *ptr = strtok((char *)lora_msg.c_str(), ", ");
        date = ptr;
        Serial.println(date);
        ptr = strtok(NULL, ", ");
        time = ptr;
        Serial.println(time);
        ptr = strtok(NULL, ", ");
        temp = ptr;
        ptr = strtok(NULL, ", ");
        TDS = ptr;
        return;
}

void task_wifi(void *p){
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ThingSpeak.begin(wifiClient);  // Initialize ThingSpeak
  String lora_msg;
  while(true){
    if (millis() - lastUpdateTime >=  postingInterval) {
      if (xQueueReceive(lora_queue, (void *)&lora_msg, 10) == pdTRUE) {
        lastUpdateTime = millis();
        Serial.println("begin split");
        /*char *ptr = strtok((char *)lora_msg.c_str(), ", ");
        char *date = ptr;
        Serial.println(date);
        ptr = strtok(NULL, ", ");
        char *time = ptr;
        Serial.println(time);
        ptr = strtok(NULL, ", ");
        char *temperaturastr = ptr;
        ptr = strtok(NULL, ", ");
        char *TDSstr = ptr;*/
        char *date, *time, *temperaturastr, *TDSstr;
        payload_separate(lora_msg, date, time, temperaturastr, TDSstr, NULL, NULL);
        Serial.println(temperaturastr);
        Serial.println("converting to float");
        float temp = atof(temperaturastr);
        Serial.println(TDSstr);
        Serial.println("converting to int");
        int TDS = atoi(TDSstr);
        Serial.println("thingspeak");
        ThingSpeak.setField(1, temp);
        ThingSpeak.setField(2, TDS);
        Serial.println("set");
        int writeSuccess = ThingSpeak.writeFields( channelID, writeAPIKey);
        Serial.println(writeSuccess);
      }
    }
  }  
}
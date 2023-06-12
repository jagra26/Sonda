#include "LoRa.h"
#include <Arduino.h>
#include <ThingSpeak.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <string.h>

// define the pins used by the transceiver module
#define ss 18
#define rst 14
#define dio0 26

#define LORA_MSG_SIZE 256
static QueueHandle_t lora_queue;

const char *ssid = "brisa-1921072";
const char *password = "to7w55gc";

// ThingSpeak information
char thingSpeakAddress[] = "api.thingspeak.com";
unsigned long channelID = 1998424;
char *readAPIKey = "OL7XLXZBSTHO55I6";
char *writeAPIKey = "849B5ZPP512LA7VS";
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
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // replace the LoRa.begin(---E-) argument with your location's frequency
  // 433E6 for Asia
  // 866E6 for Europe
  // 915E6 for North America
  while (!LoRa.begin(866E6)) {
    Serial.println(".");
    delay(500);
  }
  lora_queue = xQueueCreate(256, sizeof(String));
  xTaskCreate(&task_rx, "task_rx", 2048, NULL, 5, NULL);
  xTaskCreate(&task_wifi, "task_wifi", 2048, NULL, 5, NULL);
}

void loop() {}

void task_rx(void *p) {
  String lora_packet;
  while (true) {
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

#define MAX_SIZE 100

char** splitString(const char* input, int* numTokens) {
    char** tokens = NULL;
    char* token = NULL;
    int tokenCount = 0;
    char inputCopy[MAX_SIZE];
    strcpy(inputCopy, input);

    token = strtok(inputCopy, ",");
    while (token != NULL) {
        tokens = (char**)realloc(tokens, (tokenCount + 1) * sizeof(char*));
        tokens[tokenCount] = (char*)malloc((strlen(token) + 1) * sizeof(char));
        strcpy(tokens[tokenCount], token);
        tokenCount++;

        token = strtok(NULL, ", ");
    }

    *numTokens = tokenCount;
    return tokens;
}

void task_wifi(void *p) {
  ThingSpeak.begin(wifiClient); // Initialize ThingSpeak
  String lora_msg;
  while (true) {
    if (millis() - lastUpdateTime >= postingInterval) {
      lastUpdateTime = millis();
      if (xQueueReceive(lora_queue, (void *)&lora_msg, 10) == pdTRUE) {
        Serial.println(lora_msg);
        //  lastUpdateTime = millis();
        Serial.println("begin split");
        int numTokens =0;
        char** tokens = splitString(lora_msg.c_str(), &numTokens);
        Serial.println(tokens[2]);
        Serial.println("converting to float");
        float temp = atof(tokens[2]);
        Serial.println(tokens[3]);
        Serial.println("converting to int");
        int TDS = atoi(tokens[3]);
        Serial.println("thingspeak");
        ThingSpeak.setField(1, temp);
        ThingSpeak.setField(2, TDS);
        Serial.println("set");
        int writeSuccess = ThingSpeak.writeFields(channelID, writeAPIKey);
        Serial.println(writeSuccess);
      }
    }
    // Serial.println(millis() - lastUpdateTime);
    vTaskDelay(1000);
  }
}

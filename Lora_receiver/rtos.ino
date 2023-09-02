#include "LoRa.h"
#include "images.h"
#include <Arduino.h>
#include <ThingSpeak.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <string.h>
#include <SPI.h> 
#include <Wire.h>
#include <U8g2lib.h>
#include "utils.h"


// define the pins used by the transceiver module
#define ss 18
#define rst 14
#define dio0 26

#define LORA_MSG_SIZE 256

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

static QueueHandle_t lora_queue;
int num = 0;
char **last_send_data = splitString("00/00/0000, 00:00:00, 00.00, 000", &num);
char **last_recv_data = splitString("00/00/0000, 00:00:00, 00.00, 000", &num);

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
void task_disp(void *p);

void setup() {
  // setup display
  u8g2.begin();
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawXBM( 48, 7, ufal_width, ufal_height, ufal_bits);
  } while ( u8g2.nextPage() );
  delay(5000);
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawXBM( 0, 0, logo_width, logo_height, logo_bits);
  } while ( u8g2.nextPage() );
  delay(1000);
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawXBM( 63, 0, lacos_logo_width, lacos_logo_height, lacos_logo_bits);
  } while ( u8g2.nextPage() );
  delay(1000);
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
  xTaskCreate(&task_rx, "task_rx", 2048, NULL, 6, NULL);
  xTaskCreate(&task_wifi, "task_wifi", 2048, NULL, 5, NULL);
  xTaskCreate(&task_disp, "task_disp", 2048, NULL, 4, NULL);
}

void loop() {}

void task_rx(void *p) {
  String lora_packet;
  int numTokens =0;
  while (true) {
    int packetSize = LoRa.parsePacket();
    //Serial.println(packetSize);
    if (packetSize) {
      // received a packet
      Serial.print("Received packet '");

      // read packet
      while (LoRa.available()) {
        lora_packet = LoRa.readString();
        last_recv_data = splitString(lora_packet.c_str(), &numTokens);
        Serial.print(lora_packet);
        xQueueSend(lora_queue, (void *)&lora_packet, 10);
      }

      // print RSSI of packet
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
    }
    LoRa.read();
    vTaskDelay(100);
  }
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
        last_send_data = splitString(lora_msg.c_str(), &numTokens);
        Serial.println(last_send_data[2]);
        Serial.println("converting to float");
        float temp = atof(last_send_data[2]);
        Serial.println(last_send_data[3]);
        Serial.println("converting to int");
        int TDS = atoi(last_send_data[3]);
        Serial.println(last_send_data[4]);
        Serial.println("converting to float");
        float turbidity = atof(last_send_data[4]);
        Serial.println("thingspeak");
        ThingSpeak.setField(1, temp);
        ThingSpeak.setField(2, TDS);
        ThingSpeak.setField(3, turbidity);
        Serial.println("set");
        int writeSuccess = ThingSpeak.writeFields(channelID, writeAPIKey);
        Serial.println(writeSuccess);
      }
    }
    // Serial.println(millis() - lastUpdateTime);
    vTaskDelay(1000);
  }
}

void task_disp(void *p){
  
  while(true){    
  formatDataPage(&u8g2, "Last data received", last_recv_data);  
  delay(10000);
  formatDataPage(&u8g2, "Last data transmited", last_send_data);  
  delay(10000);
  }
}


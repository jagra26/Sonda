#include <Arduino.h>
#include <SPI.h>
#include <ThingSpeak.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <string.h>

#include "LoRa.h"
#include "images.h"
#include "utils.h"

/**
 * @brief Slave Select pin of LoRa module.
 *
 */
#define ss 18

/**
 * @brief Reset pin of LoRa module.
 *
 */
#define rst 14

/**
 * @brief DIO0 pin of LoRa module.
 *
 */
#define dio0 26

/**
 * @brief Max size of LoRa message.
 *
 */
#define LORA_MSG_SIZE 256

/**
 * @brief OLED display structure.
 *
 */
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/15, /* data=*/4,
                                         /* reset=*/16);

/**
 * @brief LoRa messages queue.
 *
 */
static QueueHandle_t lora_queue;

/**
 * @brief Array of strings representing the last send data fields.
 *
 */
char **last_send_data = splitString("00/00/0000, 00:00:00, 00.0, 000, 0, 0.0");

/**
 * @brief Array of strings representing the last received data fields.
 *
 */
char **last_recv_data = splitString("00/00/0000, 00:00:00, 00.0, 000, 0, 0.0");

/**
 * @brief Wi-Fi network name.
 *
 */
const char *ssid = "brisa-1921072";

/**
 * @brief Wi-Fi network password.
 *
 */
const char *password = "to7w55gc";

/**
 * @brief ThingSpeak API adress.
 *
 */
char thingSpeakAddress[] = "api.thingspeak.com";

/**
 * @brief ID channel from ThingSpeak.
 *
 */
unsigned long channelID = 1998424;

/**
 * @brief API key for write on channel.
 *
 */
char *writeAPIKey = "849B5ZPP512LA7VS";

/**
 * @brief Time to wait for post data on cloud. 2 minutes in miliseconds.
 *
 */
const unsigned long postingInterval = 120L * 1000;

/**
 * @brief Time from the last data update.
 *
 */
long lastUpdateTime = 0;

/**
 * @brief Client for Wi-Fi connection.
 *
 */
WiFiClient wifiClient;

/**
 * @brief Task for receiving data from LoRa.
 *
 * @param p
 */
void task_rx(void *p);

/**
 * @brief Task for posting data to cloud.
 *
 * @param p
 */
void task_wifi(void *p);

/**
 * @brief Task to show data on display.
 *
 * @param p
 */
void task_disp(void *p);

void setup() {

  // Setup display
  u8g2.begin();

  // Show UFAL logo
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawXBM(48, 7, ufal_width, ufal_height, ufal_bits);
  } while (u8g2.nextPage());
  delay(5000);

  // Show PELD logo
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawXBM(0, 0, logo_width, logo_height, logo_bits);
  } while (u8g2.nextPage());
  delay(1000);

  // Setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  Serial.begin(115200);

  // Replace the LoRa.begin(---E-) argument with your location's frequency
  // 433E6 for Asia
  // 866E6 for Europe
  // 915E6 for North America
  while (!LoRa.begin(866E6)) {
    Serial.println(".");
    delay(500);
  }

  // Init LoRa queue
  lora_queue = xQueueCreate(256, sizeof(String));

  // Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Init tasks
  xTaskCreate(&task_rx, "task_rx", 4096, NULL, 6, NULL);
  xTaskCreate(&task_wifi, "task_wifi", 2048, NULL, 5, NULL);
  xTaskCreate(&task_disp, "task_disp", 2048, NULL, 4, NULL);
}

void loop() {}

void task_rx(void *p) {
  String lora_packet;
  while (true) {
    int packetSize = LoRa.parsePacket();
    // Serial.println(packetSize);
    if (packetSize) {
      // received a packet
      Serial.print("Received packet '");

      // read packet
      while (LoRa.available()) {
        lora_packet = LoRa.readString();
        last_recv_data = splitString(lora_packet.c_str());
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
  // struct fields send_fields;

  while (true) {
    if (millis() - lastUpdateTime >= postingInterval) {
      lastUpdateTime = millis();
      if (xQueueReceive(lora_queue, (void *)&lora_msg, 10) == pdTRUE) {
        Serial.println(lora_msg);
        //  lastUpdateTime = millis();
        Serial.println("begin split");
        last_send_data = splitString(lora_msg.c_str());
        Serial.println("thingspeak");
        ThingSpeak.setField(1, (float)atof(last_send_data[2]));
        ThingSpeak.setField(2, atoi(last_send_data[3]));
        ThingSpeak.setField(3, atoi(last_send_data[4]));
        ThingSpeak.setField(4, (float)atof(last_send_data[5]));
        Serial.println("set");
        int writeSuccess = ThingSpeak.writeFields(channelID, writeAPIKey);
        Serial.println(writeSuccess);
      }
    }
    // Serial.println(millis() - lastUpdateTime);
    vTaskDelay(1000);
  }
}

void task_disp(void *p) {

  while (true) {
    formatDataPage(&u8g2, "Last data received", last_recv_data);
    delay(10000);
    formatDataPage(&u8g2, "Last data transmited", last_send_data);
    delay(10000);
    // update_display(&u8g2, 10000);
  }
}

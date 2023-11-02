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
 * @brief Pino Slave Select do módulo LoRa.
 *
 */
#define ss 18

/**
 * @brief Pino reset do módulo LoRa.
 *
 */
#define rst 14

/**
 * @brief Pino DIO0 do módulo LoRa.
 *
 */
#define dio0 26

/**
 * @brief Tamanho máximo da mensagem LoRa.
 *
 */
#define LORA_MSG_SIZE 256

/**
 * @brief Estrutura do display OLED.
 *
 */
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/15, /* data=*/4,
                                         /* reset=*/16);

/**
 * @brief Fila das mensagens LoRa.
 *
 */
static QueueHandle_t lora_queue;

/**
 * @brief Tamanho do pacote LoRa recebido.
 *
 */
int packet_size = 0;

/**
 * @brief Array de strings representando os últimos dados enviados.
 *
 */
char **last_send_data = splitString(
    "00/00/0000, 00:00:00, 00.0, 000, 0, 0.0, 0.000000, 0.000000, ¬¬",
    &packet_size);

/**
 * @brief Array de strings representando os últimos dados recebidos.
 *
 */
char **last_recv_data = splitString(
    "00/00/0000, 00:00:00, 00.0, 000, 0, 0.0, 0.000000, 0.000000, ¬¬",
    &packet_size);

/**
 * @brief Nome da rede Wi-Fi.
 *
 */
const char *ssid = "brisa-1921072";

/**
 * @brief Senha da rede Wi-Fi.
 *
 */
const char *password = "to7w55gc";

/**
 * @brief Endereço da API do Thingspeak.
 *
 */
char thingSpeakAddress[] = "api.thingspeak.com";

/**
 * @brief ID do canal ThingSpeak.
 *
 */
unsigned long channelID = 1998424;

/**
 * @brief Chave de escrita do canal na API Thingspeak.
 *
 */
char *writeAPIKey = "849B5ZPP512LA7VS";

/**
 * @brief Tempo de espera para postar na API. 2 minutos em milisegundos.
 *
 */
const unsigned long postingInterval = 120L * 1000;

/**
 * @brief Tempo desde o último update.
 *
 */
long lastUpdateTime = 0;

/**
 * @brief Cliente Wi-Fi.
 *
 */
WiFiClient wifiClient;

/**
 * @brief Tarefa de recebimento de dados LoRa.
 *
 * @param p
 */
void task_rx(void *p);

/**
 * @brief Tarefa de publicação de dados na nuvem.
 *
 * @param p
 */
void task_wifi(void *p);

/**
 * @brief Tarefa de exibição de dados no display OLED.
 *
 * @param p
 */
void task_disp(void *p);

void setup() {

  // Configuração do display OLED.
  u8g2.begin();

  // Apresentação logo da UFAL.
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawXBM(48, 7, ufal_width, ufal_height, ufal_bits);
  } while (u8g2.nextPage());
  delay(5000);

  // Apresentação logo do PELD-CCAL.
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawXBM(0, 0, logo_width, logo_height, logo_bits);
  } while (u8g2.nextPage());
  delay(1000);

  // Configuração do módulo LoRa.
  LoRa.setPins(ss, rst, dio0);
  Serial.begin(115200);

  // Substitua a frequência conforme a região.
  // 433E6 for Asia
  // 866E6 for Europe
  // 915E6 for North America
  while (!LoRa.begin(866E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.enableCrc();

  // Inicialização da fila das mensagens LoRa.
  lora_queue = xQueueCreate(256, sizeof(String));

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Inicialização de tarefas
  xTaskCreate(&task_rx, "task_rx", 4096, NULL, 6, NULL);
  xTaskCreate(&task_wifi, "task_wifi", 2048, NULL, 5, NULL);
  xTaskCreate(&task_disp, "task_disp", 2048, NULL, 4, NULL);
}

void loop() {}

void task_rx(void *p) {
  String lora_packet;
  while (true) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      // Pacote recebido.
      Serial.print("Received packet '");

      // Leitura do pacote.
      while (LoRa.available()) {
        lora_packet = LoRa.readString();
        if (check_msg(lora_packet.c_str())) {
          last_recv_data = splitString(lora_packet.c_str(), &packet_size);
          xQueueSend(lora_queue, (void *)&lora_packet, 10);
        }
        Serial.print(lora_packet);
      }

      // RSSI do pacote
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
    }
    LoRa.read();
    vTaskDelay(100);
  }
}

void task_wifi(void *p) {
// Inicialização do ThingSpeak
  ThingSpeak.begin(wifiClient); 
  String lora_msg;

  while (true) {
    if (millis() - lastUpdateTime >= postingInterval) {
      lastUpdateTime = millis();
      if (xQueueReceive(lora_queue, (void *)&lora_msg, 10) == pdTRUE) {
        Serial.println(lora_msg);
        Serial.println("begin split");
        last_send_data = splitString(lora_msg.c_str(), &packet_size);
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
    vTaskDelay(1000);
  }
}

void task_disp(void *p) {

  while (true) {
    formatDataPage(&u8g2, "Last data received", last_recv_data);
    delay(10000);
    formatDataPage(&u8g2, "Last data transmited", last_send_data);
    delay(10000);
    formatGPSPage(&u8g2, last_recv_data, last_send_data);
    delay(10000);
  }
}

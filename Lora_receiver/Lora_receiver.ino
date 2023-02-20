/*********
  Modified from the examples of the Arduino LoRa library
  More resources: https://randomnerdtutorials.com
*********/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <U8x8lib.h>
#include <time.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <string.h>
#include <ThingSpeak.h>
#include <stdlib.h>
// the OLED used
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

//define the pins used by the transceiver module
#define ss 18
#define rst 14
#define dio0 26
static const int led_pin = LED_BUILTIN;
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

void setup() {
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.clear();
  u8x8.drawString(0, 0, "LoRa Receiver");
  //initialize Serial Monitor
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Receiver");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  pinMode(led_pin, OUTPUT);

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(866E6)) {
    Serial.println(".");
    delay(500);
  }
   // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
  u8x8.clear();
  u8x8.drawString(0, 0, "LoRa Initializing OK!");
  ThingSpeak.begin(wifiClient);  // Initialize ThingSpeak
  Serial.println("Thingspeak OK!");

}

void loop() {
  // try to parse packet
  //digitalWrite(led_pin, HIGH);
  //delay(500);
  //digitalWrite(led_pin, LOW);
  int packetSize = LoRa.parsePacket();
  String LoRaData;
  if (packetSize) {
    Serial.println("");
    // received a packet
    Serial.print("Received packet '");
    u8x8.clear();
    u8x8.drawString(0, 0, "Received packet");

    // read packet
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      Serial.print(LoRaData);
      u8x8.drawString(0, 1, LoRaData.c_str()); 
    }
  if (millis() - lastUpdateTime >=  postingInterval) {
    lastUpdateTime = millis();
    Serial.println("begin split");
    char *ptr = strtok((char *)LoRaData.c_str(), ", ");
    char *date = ptr;
    Serial.println(date);
    ptr = strtok(NULL, ", ");
    char *time = ptr;
    Serial.println(time);
    ptr = strtok(NULL, ", ");
    char *temperaturastr = ptr;
    ptr = strtok(NULL, ", ");
    char *TDSstr = ptr;
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
    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
    digitalWrite(led_pin, HIGH);
    delay(500);
    digitalWrite(led_pin, LOW);
    delay(500);
  }
  Serial.print(".");
  delay(500);
}

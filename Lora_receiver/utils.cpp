#include "utils.h"

static fields recv_fields = {};
static fields send_fields = {};

char **splitString(const char *input) {
  char **tokens = NULL;
  char *token = NULL;
  int tokenCount = 0;
  char inputCopy[MAX_SIZE];
  strncpy(inputCopy, input, strlen(input));


  token = strtok(inputCopy, ",");
  while (token != NULL) {
    tokens = (char **)realloc(tokens, (tokenCount + 1) * sizeof(char *));
    tokens[tokenCount] = (char *)malloc((strlen(token) + 1) * sizeof(char));
    
    strncpy(tokens[tokenCount], token, strlen(token));
    //Serial.println(typeid(tokens[tokenCount]).name);

    tokens[tokenCount][strlen(token)] = 0;
    
    tokenCount++;

    token = strtok(NULL, ", ");
  }

  return tokens;
}

void formatDataPage(U8G2_SSD1306_128X64_NONAME_F_SW_I2C *u8g2,
                    const char *title, char** data) {
  u8g2->firstPage();
  do {
    u8g2->setFont(u8g2_font_12x6LED_tf);
    u8g2->drawStr(0, 12, title);
    u8g2->setFont(u8g2_font_NokiaSmallBold_tf);
    u8g2->drawStr(0, 24, "Date");
    u8g2->drawStr(0, 32, "Time");
    u8g2->drawStr(0, 40, "Temp");
    u8g2->drawStr(0, 48, "TDS");
    u8g2->drawStr(0, 56, "Turb");
    u8g2->drawStr(0, 64, "pH");
    u8g2->drawStr(72, 24, data[0]);
    u8g2->drawStr(86, 32, data[1]);
    u8g2->drawStr(94, 40, data[2]);
    u8g2->drawStr(122, 40, "C");
    u8g2->drawStr(90, 48, data[3]);
    u8g2->drawStr(108, 48, "ppm");
    u8g2->drawStr(120, 56, data[4]);
    u8g2->drawStr(114, 64, data[5]);
  } while (u8g2->nextPage());
}

void set_send_fields(char *date, char *time, int TDS, int turbidity,
                     float temperature, float pH) {

  strncpy(send_fields.date, date, strlen(date));
  strncpy(send_fields.time, time, strlen(time));
  send_fields.temperature = temperature;
  send_fields.TDS = TDS;
  send_fields.turbidity = turbidity;
  send_fields.pH = pH;
}

void set_recv_fields(char *date, char *time, int TDS, int turbidity,
                     float temperature, float pH) {

  strncpy(recv_fields.date, date, strlen(date));
  strncpy(recv_fields.time, time, strlen(time));
  recv_fields.temperature = temperature;
  recv_fields.TDS = TDS;
  recv_fields.turbidity = turbidity;
  recv_fields.pH = pH;
}

void get_send_fields(struct fields *fields) {
  memcpy(fields, &send_fields, sizeof(struct fields));
}

void update_display(U8G2_SSD1306_128X64_NONAME_F_SW_I2C *u8g2, int delay_ms) {
  u8g2->firstPage();
  do {
    u8g2->setFont(u8g2_font_12x6LED_tf);
    u8g2->drawStr(0, 12, "Last data received");
    u8g2->setFont(u8g2_font_NokiaSmallBold_tf);
    u8g2->drawStr(0, 24, "Date");
    u8g2->drawStr(0, 32, "Time");
    u8g2->drawStr(0, 40, "Temp");
    u8g2->drawStr(0, 48, "TDS");
    u8g2->drawStr(0, 56, "Turb");
    u8g2->drawStr(0, 64, "pH");
    u8g2->drawStr(72, 24, recv_fields.date);
    u8g2->drawStr(86, 32, recv_fields.time);
    u8g2->drawStr(94, 40, String(recv_fields.temperature).c_str());
    u8g2->drawStr(122, 40, "C");
    u8g2->drawStr(90, 48, String(recv_fields.TDS).c_str());
    u8g2->drawStr(108, 48, "ppm");
    u8g2->drawStr(118, 56, String(recv_fields.turbidity).c_str());
    u8g2->drawStr(114, 64, String(recv_fields.pH).c_str());
  } while (u8g2->nextPage());
  delay(delay_ms);
  u8g2->firstPage();
  do {
    u8g2->setFont(u8g2_font_12x6LED_tf);
    u8g2->drawStr(0, 12, "Last data received");
    u8g2->setFont(u8g2_font_NokiaSmallBold_tf);
    u8g2->drawStr(0, 24, "Date");
    u8g2->drawStr(0, 32, "Time");
    u8g2->drawStr(0, 40, "Temp");
    u8g2->drawStr(0, 48, "TDS");
    u8g2->drawStr(0, 56, "Turb");
    u8g2->drawStr(0, 64, "pH");
    u8g2->drawStr(72, 24, send_fields.date);
    u8g2->drawStr(86, 32, send_fields.time);
    u8g2->drawStr(94, 40, String(send_fields.temperature).c_str());
    u8g2->drawStr(122, 40, "C");
    u8g2->drawStr(90, 48, String(send_fields.TDS).c_str());
    u8g2->drawStr(108, 48, "ppm");
    u8g2->drawStr(118, 56, String(send_fields.turbidity).c_str());
    u8g2->drawStr(114, 64, String(send_fields.pH).c_str());
  } while (u8g2->nextPage());
  delay(delay_ms);
}
#include "utils.h"

char **splitString(const char *input, int *packet_size) {
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

    tokens[tokenCount][strlen(token)] = 0;

    tokenCount++;

    token = strtok(NULL, ", ");
  }
  *packet_size = tokenCount;
  return tokens;
}

void formatDataPage(U8G2_SSD1306_128X64_NONAME_F_SW_I2C *u8g2,
                    const char *title, char **data) {
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

bool check_msg(const char *lora_msg) {
  int packet_size = 0;
  char **result = splitString(lora_msg, &packet_size);
  if (packet_size == PACKET_FIELDS &&
      strcmp(result[packet_size - 1], "¬¬") == 0) {
    return true;
  }
  return false;
}

void formatGPSPage(U8G2_SSD1306_128X64_NONAME_F_SW_I2C *u8g2, char **data_recv, char **data_send) {
  u8g2->firstPage();
  do {
    u8g2->setFont(u8g2_font_12x6LED_tf);
    u8g2->drawStr(0, 12, "Last coords received");
    u8g2->setFont(u8g2_font_NokiaSmallBold_tf);
    u8g2->drawStr(0, 24, "Latitude");
    u8g2->drawStr(0, 32, "Longitude");
    u8g2->drawStr(79, 24, data_recv[6]);
    u8g2->drawStr(79, 32, data_recv[7]);
    u8g2->setFont(u8g2_font_12x6LED_tf);
    u8g2->drawStr(0, 45, "Last coords sended");
    u8g2->setFont(u8g2_font_NokiaSmallBold_tf);
    u8g2->drawStr(0, 55, "Latitude");
    u8g2->drawStr(0, 63, "Longitude");
    u8g2->drawStr(79, 55, data_send[6]);
    u8g2->drawStr(79, 63, data_send[7]);
    
  } while (u8g2->nextPage());
}
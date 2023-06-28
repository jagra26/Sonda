#include "utils.h"
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

void formatDataPage(U8G2_SSD1306_128X64_NONAME_F_SW_I2C* u8g2, const char* title, char** data){
  u8g2->firstPage();
  do {
    u8g2->setFont(u8g2_font_12x6LED_tf);
    u8g2->drawStr(0,12,title);
    u8g2->setFont(u8g2_font_NokiaSmallBold_tf);
    u8g2->drawStr(0,24,"Date");
    u8g2->drawStr(0,32,"Time");
    u8g2->drawStr(0,40,"Temp");
    u8g2->drawStr(0,48,"TDS");
    u8g2->drawStr(0,56,"pH");
    u8g2->drawStr(0,64,"Turb");
    u8g2->drawStr(72,24,data[0]);
    u8g2->drawStr(86,32,data[1]);
    u8g2->drawStr(94,40,data[2]);
    u8g2->drawStr(122,40,"C");
    u8g2->drawStr(90,48,data[3]);
    u8g2->drawStr(108,48,"ppm");
    u8g2->drawStr(110,56,"pH");
    u8g2->drawStr(98,64,"Turb");
  } while ( u8g2->nextPage() );
}
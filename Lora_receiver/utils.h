#include <cstddef>
#include <string.h>
#include <cstdlib>
#include <U8g2lib.h>

#define MAX_SIZE 100

char** splitString(const char* input, int* numTokens);

void formatDataPage(U8G2_SSD1306_128X64_NONAME_F_SW_I2C* u8g2, const char* title, char** data);
#include <U8g2lib.h>
#include <cstddef>
#include <cstdlib>
#include <string.h>

#define MAX_SIZE 100

struct fields {
  int TDS;
  int turbidity;
  float temperature;
  float pH;
  char *date;
  char *time;
};

char **splitString(const char *input);

void formatDataPage(U8G2_SSD1306_128X64_NONAME_F_SW_I2C *u8g2,
                    const char *title, char** data);

void set_send_fields(char *date, char *time, int TDS, int turbidity,
                     float temperature, float pH);

void set_recv_fields(char *date, char *time, int TDS, int turbidity,
                     float temperature, float pH);

void get_send_fields(struct fields *fields);

void update_display(U8G2_SSD1306_128X64_NONAME_F_SW_I2C *u8g2, int delay_ms);
#include "ADS1115_lite.h"

#define ADS1115_RESOLUTION 0.1875
#define MILI_TO_VOLT 1000

float digit_to_voltage(int16_t adc_read);

int16_t ads_read(ADS1115_lite *ads, int mux_param);

void ads_config(ADS1115_lite *ads, int gain_param, int sample_rate);
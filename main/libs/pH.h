
#define VOLTAGE_7_0 2.7
#define VOLTAGE_4_0_1 3.1
#define VOLTAGE_6_8_6 2.6
#define a_4_0_1 (VOLTAGE_7_0 - VOLTAGE_4_0_1) / (7 - 4.01)
#define a_6_8_6 (VOLTAGE_7_0 - VOLTAGE_6_8_6) / (7 - 6.86)
#define a_m (a_4_0_1 + a_6_8_6) / 2
#define b 0

float ph_calc(float voltage);
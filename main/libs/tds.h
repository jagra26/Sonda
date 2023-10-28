#define LINEAR_COEF_TEMP_COEF 1.0
#define ANGULAR_COEF_TEMP_COEF 0.02
#define TEMP_CONSTANT 25.0
#define CUBIC_COEF 133.42
#define SQUARE_COEF 255.86
#define LINEAR_COEF 857.39
#define LINEAR_CALIB_COEF 37.67109365977577
#define ANGULAR_CALIB_COEF 0.7073730057908374

int tds_calc(float voltage, float temperature);
int tds_calib(int tds);
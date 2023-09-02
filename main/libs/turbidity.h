
#define DELTA 0.0

float calculate_delta_u(float temp);

float calculate_u_25(float voltage, float delta);

float calculate_K(float u_25);

float calculate_turbidity(float voltage, float K);

float get_turbidity(float voltage);
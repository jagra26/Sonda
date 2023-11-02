#include <U8g2lib.h>
#include <cstddef>
#include <cstdlib>
#include <string.h>

/**
 * @brief Tamanho máximo de um pacote LoRa.
 *
 */
#define MAX_SIZE 100

/**
 * @brief Número de campos de um pacote válido.
 *
 */
#define PACKET_FIELDS 9

/**
 * @brief Separa um pacote em uma lista de strings. Retorna o tamanho da lista
 * em packet_size.
 *
 * @param input Pacote LoRa.
 * @param packet_size[out] Tamanho da lista de strings.
 * @return char** Lista de strings que representam os campos.
 */
char **splitString(const char *input, int *packet_size);

/**
 * @brief Desenha uma página de dados no display.
 *
 * @param u8g2 Estrutura que guarda as informações do display.
 * @param title Título da página.
 * @param data Lista de strings que representam os campos.
 */
void formatDataPage(U8G2_SSD1306_128X64_NONAME_F_SW_I2C *u8g2,
                    const char *title, char **data);

/**
 * @brief Checa se um pacote LoRa é válido e deve ser processado.
 *
 * @param lora_msg Pacote LoRa.
 * @return true Caso seja válido.
 * @return false Caso contrário.
 */
bool check_msg(const char *lora_msg);

/**
 * @brief Desenha uma página de coordenadas no display.
 *
 * @param u8g2 Estrutura que guarda as informações do display.
 * @param data_recv Lista de strings que representam os últimos campos
 * recebidos.
 * @param data_send Lista de strings que representam os últimos campos
 * transmitidos.
 */
void formatGPSPage(U8G2_SSD1306_128X64_NONAME_F_SW_I2C *u8g2, char **data_recv,
                   char **data_send);
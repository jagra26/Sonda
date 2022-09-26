// sd card
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>


static const char *SDTAG = "SD CARD";
#define SD true
#define MOUNT_POINT "/sdcard"
#define SDSPI_DEFAULT_DMA 1

/**
 * @brief Function to initialize the SD card
 *
 * @param PIN_NUM_MOSI
 * @param PIN_NUM_MISO
 * @param PIN_NUM_CLK
 * @param PIN_NUM_CS
 */
void mount_file_system(int PIN_NUM_MOSI, int PIN_NUM_MISO, int PIN_NUM_CLK,
                       int PIN_NUM_CS);
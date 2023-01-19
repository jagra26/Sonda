/**
 * @file SD_utils.h
 * @author João Almeia (jagra@ic.ufal.br)
 * @author Rui Santos
 * @brief Library with methods for access and operate a sd card
 * @version 0.1
 * @date 2022-11-09
 * @note This code was based on the work of Rui Santos at
 * https://RandomNerdTutorials.com/esp32-microsd-card-arduino/
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "FS.h"
#include "SD.h"

static const char *SDTAG = "SD CARD";

void checkSD( uint8_t cardType);

void listDir(fs::FS &fs, const char *dirname, uint8_t levels);

void createDir(fs::FS &fs, const char *path);

void removeDir(fs::FS &fs, const char *path);

void readFile(fs::FS &fs, const char *path);

void writeFile(fs::FS &fs, const char *path, const char *message);

void appendFile(fs::FS &fs, const char *path, const char *message);

void renameFile(fs::FS &fs, const char *path1, const char *path2);

void deleteFile(fs::FS &fs, const char *path);

void testFileIO(fs::FS &fs, const char *path);
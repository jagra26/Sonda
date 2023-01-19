#include "SD_utils.h"

void checkSD(uint8_t cardType){
  if (cardType == CARD_NONE) {
    ESP_LOGE(SDTAG, "No SD card attached");
    return;
  }

  ESP_LOGI(SDTAG, "SD Card Type: ");
  switch (cardType) {
  {
  case CARD_MMC:
    ESP_LOGI(SDTAG, "MMC");
    break;
  case CARD_SD:
    ESP_LOGI(SDTAG, "SDSC");
    break;
  case CARD_SDHC:
    ESP_LOGI(SDTAG, "SDHC");
    break;
  default:
    ESP_LOGI(SDTAG, "UNKNOWN");
    break;
  }
}
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  ESP_LOGI(SDTAG, "Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    ESP_LOGE(SDTAG, "Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    ESP_LOGE(SDTAG, "Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      ESP_LOGI(SDTAG, "  DIR : %s\n", file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      ESP_LOGI(SDTAG, "  FILE : %s\n", file.name());
      ESP_LOGI(SDTAG, "  SIZE : %s\n", file.name());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  ESP_LOGI(SDTAG, "Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    ESP_LOGI(SDTAG, "Dir created");
  } else {
    ESP_LOGE(SDTAG, "mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  ESP_LOGI(SDTAG, "Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    ESP_LOGI(SDTAG, "Dir removed");
  } else {
    ESP_LOGE(SDTAG, "rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {
  ESP_LOGI(SDTAG, "Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    ESP_LOGE(SDTAG, "Failed to open file for reading");
    return;
  }

  ESP_LOGI(SDTAG, "Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
    //ESP_LOGI(SDTAG, file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  ESP_LOGI(SDTAG, "Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    ESP_LOGE(SDTAG, "Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    ESP_LOGI(SDTAG, "File written");
  } else {
    ESP_LOGE(SDTAG, "Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  ESP_LOGI(SDTAG, "Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    ESP_LOGE(SDTAG, "Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    ESP_LOGI(SDTAG, "Message appended");
  } else {
    ESP_LOGE(SDTAG, "Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  ESP_LOGI(SDTAG, "Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    ESP_LOGI(SDTAG, "File renamed");
  } else {
    ESP_LOGE(SDTAG, "Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  ESP_LOGI(SDTAG, "Deleting file: %s\n", path);
  if (fs.remove(path)) {
    ESP_LOGI(SDTAG, "File deleted");
  } else {
    ESP_LOGE(SDTAG, "Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    ESP_LOGI(SDTAG, "%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    ESP_LOGE(SDTAG, "Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file) {
    ESP_LOGE(SDTAG, "Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  ESP_LOGI(SDTAG, "%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}

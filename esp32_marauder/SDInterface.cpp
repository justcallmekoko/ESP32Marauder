#include "SDInterface.h"
#include "lang_var.h"

// GCOVR_EXCL_START -- requires mounted SPIFFS and SD filesystems.
namespace {
  bool ensureDirectory(fs::FS& fs, const String& path) {
    if (path.length() == 0 || path == "/" || fs.exists(path))
      return true;

    int slash = path.lastIndexOf('/');
    if (slash > 0 && !ensureDirectory(fs, path.substring(0, slash)))
      return false;

    return fs.mkdir(path);
  }

  bool removeTree(fs::FS& fs, const String& path) {
    if (!fs.exists(path))
      return true;

    File node = fs.open(path);
    if (!node)
      return false;

    if (!node.isDirectory()) {
      node.close();
      return fs.remove(path);
    }

    File child = node.openNextFile();
    while (child) {
      String child_path = child.path();
      child.close();
      if (!removeTree(fs, child_path)) {
        node.close();
        return false;
      }
      child = node.openNextFile();
    }

    node.close();
    return fs.rmdir(path);
  }

  bool copyTree(
    fs::FS& source,
    const String& source_path,
    fs::FS& destination,
    const String& destination_path,
    size_t& files_copied,
    size_t& bytes_copied,
    String& error
  ) {
    File source_node = source.open(source_path);
    if (!source_node) {
      error = "Could not open SPIFFS path " + source_path;
      return false;
    }

    if (!source_node.isDirectory()) {
      int slash = destination_path.lastIndexOf('/');
      if (slash > 0 && !ensureDirectory(destination, destination_path.substring(0, slash))) {
        source_node.close();
        error = "Could not create SD backup directory";
        return false;
      }

      File destination_file = destination.open(destination_path, FILE_WRITE);
      if (!destination_file) {
        source_node.close();
        error = "Could not create " + destination_path;
        return false;
      }

      uint8_t buffer[512];
      while (source_node.available()) {
        size_t bytes_read = source_node.read(buffer, sizeof(buffer));
        if (bytes_read == 0 || destination_file.write(buffer, bytes_read) != bytes_read) {
          source_node.close();
          destination_file.close();
          error = "Failed while copying " + source_path;
          return false;
        }
        bytes_copied += bytes_read;
      }

      source_node.close();
      destination_file.close();
      files_copied++;
      return true;
    }

    if (!ensureDirectory(destination, destination_path)) {
      source_node.close();
      error = "Could not create " + destination_path;
      return false;
    }

    File child = source_node.openNextFile();
    while (child) {
      String child_source_path = child.path();
      String child_name = child_source_path;
      if (child_name.startsWith(source_path))
        child_name.remove(0, source_path.length());
      while (child_name.startsWith("/"))
        child_name.remove(0, 1);
      child.close();

      if (!copyTree(
        source,
        child_source_path,
        destination,
        destination_path + "/" + child_name,
        files_copied,
        bytes_copied,
        error
      )) {
        source_node.close();
        return false;
      }
      child = source_node.openNextFile();
    }

    source_node.close();
    return true;
  }
}
// GCOVR_EXCL_STOP

#ifdef HAS_C5_SD
  SDInterface::SDInterface(SPIClass* spi, int cs)
    : _spi(spi), _cs(cs) {}
#endif

bool SDInterface::initSD() {
  #ifdef HAS_SD
    String display_string = "";

    #ifdef KIT
      pinMode(SD_DET, INPUT);
      if (digitalRead(SD_DET) != LOW) {
        this->supported = false;
        return false;
      }
    #endif

    pinMode(SD_CS, OUTPUT);

    delay(10);
    #if (defined(MARAUDER_M5STICKC)) || (defined(HAS_CYD_TOUCH)) || (defined(MARAUDER_CARDPUTER)) || (defined(MARAUDER_CARDPUTER_ADV))
      /* Set up SPI SD Card using external pin header
      StickCPlus Header - SPI SD Card Reader
                  3v3   -   3v3
                  GND   -   GND
                   G0   -   CLK
              G36/G25   -   MISO
                  G26   -   MOSI
                        -   CS (jumper to SD Card GND Pin)
      */
      #if defined(MARAUDER_M5STICKC)
        enum { SPI_SCK = 0, SPI_MISO = 36, SPI_MOSI = 26 };
      #elif defined(HAS_CYD_TOUCH) || defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV) || defined(HAS_SEPARATE_SD)
        enum { SPI_SCK = SD_SCK, SPI_MISO = SD_MISO, SPI_MOSI = SD_MOSI };
      #else
        enum { SPI_SCK = 0, SPI_MISO = 36, SPI_MOSI = 26 };
      #endif
      #if !defined(MARAUDER_CARDPUTER) && !defined(MARAUDER_CARDPUTER_ADV)
        this->spiExt = new SPIClass();
      #else
        this->spiExt = new SPIClass(FSPI);
      #endif
      Serial.println(F("Using external SPI configuration..."));
      this->spiExt->begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
      if (!SD.begin(SD_CS, *(this->spiExt))) {
    #elif defined(HAS_C5_SD)
      if (!SD.begin(SD_CS, *_spi)) {
    #else
      if (!SD.begin(SD_CS)) {
    #endif
      Serial.println(F("Failed to mount SD Card"));
      this->supported = false;
      return false;
    }
    else {
      this->supported = true;
      this->cardType = SD.cardType();

      this->cardSizeMB = SD.cardSize() / (1024 * 1024);
    
      if (this->supported) {
        const int NUM_DIGITS = log10(this->cardSizeMB) + 1;

        char sz[NUM_DIGITS + 1];

        sz[NUM_DIGITS] =  0;
        for ( size_t i = NUM_DIGITS; i--; this->cardSizeMB /= 10)
        {
            sz[i] = '0' + (this->cardSizeMB % 10);
            display_string.concat((String)sz[i]);
        }
  
        this->card_sz = sz;
      }

      if (!SD.exists("/SCRIPTS")) {

        SD.mkdir("/SCRIPTS");
      }

      this->sd_files = new LinkedList<String>();
    
      return true;
  }

  #else
    return false;
  #endif
}

File SDInterface::getFile(String path) {
  if (this->supported) {
    File file = SD.open(path, FILE_READ);

    //if (file)
    return file;
  }
}

bool SDInterface::removeFile(String file_path) {
  if (SD.remove(file_path))
    return true;
  else
    return false;
}

// GCOVR_EXCL_START -- requires mounted SPIFFS and SD filesystems.
bool SDInterface::backupSPIFFS(size_t& files_copied, size_t& bytes_copied, String& error) {
  files_copied = 0;
  bytes_copied = 0;
  error = "";

  if (!this->supported) {
    error = "SD card not detected";
    return false;
  }

  const String backup_path = "/spiffs";
  const String staging_path = "/spiffs.tmp";
  const String previous_path = "/spiffs.previous";

  if (!removeTree(SD, staging_path) || !removeTree(SD, previous_path)) {
    error = "Could not clear old SPIFFS backup data";
    return false;
  }

  if (!copyTree(SPIFFS, "/", SD, staging_path, files_copied, bytes_copied, error)) {
    removeTree(SD, staging_path);
    return false;
  }

  if (SD.exists(backup_path) && !SD.rename(backup_path, previous_path)) {
    removeTree(SD, staging_path);
    error = "Could not rotate existing SPIFFS backup";
    return false;
  }

  if (!SD.rename(staging_path, backup_path)) {
    if (SD.exists(previous_path))
      SD.rename(previous_path, backup_path);
    error = "Could not activate new SPIFFS backup";
    return false;
  }

  removeTree(SD, previous_path);
  return true;
}
// GCOVR_EXCL_STOP

void SDInterface::listDirToLinkedList(LinkedList<String>* file_names, String str_dir, String ext) {
  if (this->supported) {
    File dir = SD.open(str_dir);
    while (true)
    {
      File entry = dir.openNextFile();
      if (!entry)
      {
        break;
      }

      if (entry.isDirectory())
        continue;

      String file_name = entry.name();
      if (ext != "") {
        if (file_name.endsWith(ext)) {
          file_names->add(file_name);
        }
      }
      else
        file_names->add(file_name);
    }
  }
}

void SDInterface::listDir(String str_dir){
  if (this->supported) {
    File dir = SD.open(str_dir);
    while (true)
    {
      File entry = dir.openNextFile();
      if (! entry)
      {
        break;
      }
      //for (uint8_t i = 0; i < numTabs; i++)
      //{
      //  Serial.print('\t');
      //}
      Serial.print(entry.name());
      Serial.print("\t");
      Serial.println(entry.size());
      entry.close();
    }
  }
}

void SDInterface::runUpdate(String file_name) {
  if (file_name == "")
    file_name = "/update.bin";

  #ifdef HAS_SCREEN
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setCursor(0, TFT_HEIGHT / 3);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_WHITE);
  
    display_obj.tft.println("Opening " + file_name + "...");
  #endif

  File updateBin = SD.open(file_name);

  if (updateBin) {
    if(updateBin.isDirectory()){
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_RED);
        display_obj.tft.println(F(text_table2[0]));
      #endif
      Serial.print(F("Error, could not find \""));
      Serial.print(file_name);
      Serial.println(F("\""));
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_WHITE);
      #endif
      updateBin.close();
      return;
    }

    size_t updateSize = updateBin.size();

    if (updateSize > 0) {
      #ifdef HAS_SCREEN
        display_obj.tft.println(F(text_table2[1]));
      #endif
      Serial.println(F("Starting update over SD. Please wait..."));
      this->performUpdate(updateBin, updateSize);
    }
    else {
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_RED);
        display_obj.tft.println(F(text_table2[2]));
      #endif
      Serial.println(F("Error, file is empty"));
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_WHITE);
      #endif
      return;
    }

    updateBin.close();
    
      // whe finished remove the binary from sd card to indicate end of the process
    #ifdef HAS_SCREEN
      display_obj.tft.println(F(text_table2[3]));
    #endif
    const esp_partition_t *running = esp_ota_get_running_partition();

    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);

    esp_err_t result = esp_ota_set_boot_partition(next);
     
    ESP.restart();
  }
  else {
    #ifdef HAS_SCREEN
      display_obj.tft.setTextColor(TFT_RED);
      display_obj.tft.println(F(text_table2[4]));
    #endif
    Serial.println(F("Could not load update.bin from sd root"));
    #ifdef HAS_SCREEN
      display_obj.tft.setTextColor(TFT_WHITE);
    #endif
  }
}

void SDInterface::performUpdate(Stream &updateSource, size_t updateSize) {
  if (Update.begin(updateSize)) {   
    #ifdef HAS_SCREEN
      display_obj.tft.println(text_table2[5] + String(updateSize));
      display_obj.tft.println(F(text_table2[6]));
    #endif
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize) {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table2[7] + String(written) + text_table2[10]);
      #endif
      Serial.print(F("Written : "));
      Serial.print(written);
      Serial.println(F(" successfully"));
    }
    else {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table2[8] + String(written) + "/" + String(updateSize) + text_table2[9]);
      #endif
      Serial.print(F("Written only : "));
      Serial.print(written);
      Serial.print(F("/"));
      Serial.print(updateSize);
      Serial.println(F(". Retry?"));
    }
    if (Update.end()) {
      if (Update.isFinished()) {

      }
      else {
        #ifdef HAS_SCREEN
          display_obj.tft.setTextColor(TFT_RED);
          display_obj.tft.println(text_table2[12]);
        #endif
        Serial.println(F("Update not finished? Something went wrong!"));
        #ifdef HAS_SCREEN
          display_obj.tft.setTextColor(TFT_WHITE);
        #endif
      }
    }
    else {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table2[13] + String(Update.getError()));
      #endif
      Serial.print(F("Error Occurred. Error #: "));
      Serial.println(Update.getError());
    }

  }
  else
  {
    #ifdef HAS_SCREEN
      display_obj.tft.println(text_table2[14]);
    #endif
    Serial.println(F("Not enough space to begin OTA"));
  }
}

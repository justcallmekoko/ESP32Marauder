#pragma once

#ifndef SDInterface_h
#define SDInterface_h

#include "configs.h"

#include "settings.h"
#ifdef HAS_C5_SD
  #include "FS.h"
#endif

#ifdef HAS_SDMMC
  #include <SD_MMC.h>
//  extern fs::SDMMCFS& SD = SD_MMC;
  #define SD SD_MMC    // preprocessor substitution, not a variable definition
#else
  #include "SD.h"
#endif

#ifdef HAS_C5_SD
  #include "SPI.h"
#endif
#include "Buffer.h"
#ifdef HAS_SCREEN
  #include "Display.h"
#endif
#include <Update.h>

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_err.h"

extern Buffer buffer_obj;
extern Settings settings_obj;
#ifdef HAS_SCREEN
  extern Display display_obj;
#endif

#ifdef KIT
  #define SD_DET 4
#endif

class SDInterface {

  private:
  #if (defined(MARAUDER_M5STICKC) || defined(HAS_CYD_TOUCH) || defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV))
    SPIClass *spiExt;
  #elif defined(HAS_C5_SD)
    SPIClass* _spi;
    int _cs;
  #endif

  public:
    #ifdef HAS_C5_SD
      SDInterface(SPIClass* spi, int cs);
    #endif

    uint8_t cardType;
    //uint64_t cardSizeBT;
    //uint64_t cardSizeKB;
    uint64_t cardSizeMB;
    //uint64_t cardSizeGB;
    bool supported = false;

    String card_sz;
    String selected_file_name = "";
  
    bool initSD();

    LinkedList<String>* sd_files;

    void listDir(String str_dir);
    void listDirToLinkedList(LinkedList<String>* file_names, String str_dir = "/", String ext = "");
    File getFile(String path);
    void runUpdate(String file_name = "");
    void performUpdate(Stream &updateSource, size_t updateSize);
    bool removeFile(String file_path);
};

#endif

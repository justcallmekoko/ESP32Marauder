#pragma once

#ifndef SDInterface_h
#define SDInterface_h

#include "configs.h"

#include "settings.h"
#ifdef HAS_C5_SD
  #include "FS.h"
#endif
#include "SD.h"
#ifdef MARAUDER_FREENOVE_FNK0104B
  // The microSD slot of the Freenove FNK0104B is wired to the SDMMC
  // peripheral (1-bit: CLK=38, CMD=40, D0=39), not to the SPI bus. The whole
  // codebase talks to the SPI `SD` object, so redirect that token to SD_MMC.
  // Both the include and the alias are compiled only for this target: every
  // other board keeps using the original SPI SD object.
  #include "SD_MMC.h"
  #define SD SD_MMC
#endif
#include "SPIFFS.h"
#ifdef HAS_C5_SD
  #include "SPI.h"
#endif
#include "Buffer.h"
#include "FirmwareMetadata.h"
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

struct SDDirectoryEntry {
  String name;
  String path;
  bool is_directory;
};

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

    bool validateUpdate(File &updateBin);

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
    bool listDirectory(String path, LinkedList<SDDirectoryEntry>* entries);
    File getFile(String path);
    void runUpdate(String file_name = "");
    bool performUpdate(Stream &updateSource, size_t updateSize);
    bool removeFile(String file_path);
    bool migrateSPIFFS(uint8_t operation, size_t& files, size_t& bytes, uint8_t& error);
};

#endif

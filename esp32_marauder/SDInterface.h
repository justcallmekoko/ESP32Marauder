#pragma once

#ifndef SDInterface_h
#define SDInterface_h

#include "configs.h"

#include "settings.h"
#ifdef HAS_C5_SD
  #include "FS.h"
#endif

// SDInterface.h
#ifdef HAS_SDMMC
  #include <SD_MMC.h>
  //  extern fs::SDMMCFS& SD = SD_MMC;
    #define SD SD_MMC    // preprocessor substitution, not a variable definition
#else
  #include "SD.h"
#endif

// #include "SD.h"

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

#if defined(HAS_SDMMC) && defined(USE_MMC_WRITE_SECTORS)
  // #include "diskio_sdmmc.h" 
  // extern sdmmc_card_t* _mmc_card = nullptr;
#endif

#if defined(MSC_SHARE)
  // #include "MSC_Share.h"

  class MSC_Share;
  extern MSC_Share MSC_Share_obj;
#endif

#if defined(HAS_SDMMC) && defined(USE_MMC_WRITE_SECTORS)
  #include <SD_MMC.h>
  #include "driver/sdmmc_types.h"

  // Subclass purely to expose private _card — no data members added,
  // layout identical to SDMMCFS, reinterpret_cast is safe here
  class SDMMCFS_CardAccessor : public fs::SDMMCFS {
  public:
    sdmmc_card_t* getCard() { return _card; }
  };

  inline sdmmc_card_t* sdmmc_get_card_handle() {
    return reinterpret_cast<SDMMCFS_CardAccessor*>(&SD_MMC)->getCard();
  }
#endif

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

    void shutdownSD();     // NEW cleanly tears down whichever backend is active
    void reinitSD();       // NEW brings it back up after MSC hands control back

    // #if defined(MSC_SHARE) && defined(USE_MMC_WRITE_SECTORS)
    //   sdmmc_card_t* sdmmc_card = nullptr;
    // #endif



    uint8_t cardType;
    //uint64_t cardSizeBT;
    //uint64_t cardSizeKB;
    uint64_t cardSizeMB;
    //uint64_t cardSizeGB;
    bool supported = false;

    String card_sz;
    bool initSD();

    String selected_file_name = "";

    LinkedList<String>* sd_files;

    void listDir(String str_dir);
    void listDirToLinkedList(LinkedList<String>* file_names, String str_dir = "/", String ext = "");
    File getFile(String path);
    void runUpdate(String file_name = "");
    void performUpdate(Stream &updateSource, size_t updateSize);
    bool removeFile(String file_path);
};

#endif

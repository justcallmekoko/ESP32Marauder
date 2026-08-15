#pragma once

#ifndef MSC_Share_h
#define MSC_Share_h

#include "configs.h"

#if  defined(MSC_SHARE)

#include "Arduino.h"
#include "USB.h"
#include "USBMSC.h"
// #include "SPI.h"
// #include "SdFat.h"

#if defined(HAS_SDMMC)
  #include "driver/sdmmc_host.h"
  #ifdef HAS_IDF_3
    #include "sdmmc_cmd.h"           // sdmmc_read_sectors / sdmmc_write_sectors live here on some IDF versions
  #endif
#else
  #undef USE_MMC_WRITE_SECTORS
#endif

#include "SDInterface.h"

class SDInterface;;
extern SDInterface sd_obj;

//SDInterface sd_obj;

#if defined(HAS_SDMMC) && defined( USE_MMC_WRITE_SECTORS)
  sdmmc_card_t* _mmc_card = nullptr;
#endif

class MSC_Share {

  public:

    bool sd_ready = true;
    bool msc_active = false;
    bool msc_started = false;
    
    void RunSetup();
    void msc_start();
    void msc_pause();
    void ShareEnd();

    // friend int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize);
    // friend int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize);
    friend int32_t onWrite(uint32_t, uint32_t, uint8_t*, uint32_t);
    friend int32_t onRead(uint32_t, uint32_t, void*, uint32_t);
    friend bool onStartStop(uint8_t, bool, bool);

  private:


    // int32_t onWrite(auto... argsk);
    // int32_t onRead(auto... argsk);
    // static bool onStartStop(auto... argsk);

};

extern MSC_Share MSC_Share_obj;


#endif  // MSC_SHARE

#endif // MSC_Share_h


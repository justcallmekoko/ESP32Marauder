
#include "configs.h"

#if defined(MSC_SHARE)

#include "MSC_Share.h"
#include "esp_task_wdt.h"

// SOC_USB_OTG_SUPPORTED
// CONFIG_TINYUSB_MSC_ENABLED

USBMSC   MSC;

    // Called by USB host when it wants to WRITE a sector
    int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
      uint32_t secSize = SD.sectorSize();
      if (!secSize) return -1;

      #if defined(HAS_SDMMC) && defined(USE_MMC_WRITE_SECTORS)
        if (_mmc_card != nullptr) {
          // esp_task_wdt_reset();
          esp_err_t err = sdmmc_write_sectors(_mmc_card, buffer, lba, bufsize / secSize);
          // esp_task_wdt_reset();
          if (err != ESP_OK) {
            log_e("sdmmc_write_sectors failed: %s", esp_err_to_name(err));
            return -1;
          }
          delay(1);
          return bufsize;
        }
      #endif


      // SPI SD:  no multi-sector IDF API available, fall back to per-sector writeRAW
      for (int x = 0; x < bufsize / secSize; x++) {
        uint8_t blkbuffer[secSize];
        // esp_task_wdt_reset();
       memcpy(blkbuffer, (uint8_t *)buffer + secSize * x, secSize);
        if (!SD.writeRAW(blkbuffer, lba + x)) return -1;
      }

      return bufsize;
    }


    // Called by USB host when it wants to READ a sector
    int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
      uint32_t secSize = SD.sectorSize();
      if (!secSize) return -1;

      #if defined(HAS_SDMMC) && defined(USE_MMC_WRITE_SECTORS)
        if (_mmc_card != nullptr) {
          // esp_task_wdt_reset();
          esp_err_t err = sdmmc_read_sectors(_mmc_card, buffer, lba, bufsize / secSize);
          // esp_task_wdt_reset();
          if (err != ESP_OK) {
            log_e("sdmmc_read_sectors failed: %s", esp_err_to_name(err));
            return -1;
          }
          delay(1);
          return bufsize;
        }
      #endif

      for (int x = 0; x < bufsize / secSize; x++) {
        // esp_task_wdt_reset();
        if (!SD.readRAW((uint8_t *)buffer + (x * secSize), lba + x)) return -1;
      }

      return bufsize;
    }



  bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
    log_d("onStartStop: power_condition =  %u  start = %u load_eject = %u\n", power_condition, start, load_eject);

    if (load_eject == true && start == false) {
      log_d("MSC detached by host");
      MSC_Share_obj.msc_pause();
    }
    return true;
  }

  // Un pause SD Share
  // Call this to hand SD to USB host
  void MSC_Share::msc_start() {
    if (msc_started) {
      log_d("msc_start");
      // SD.card()->syncDevice();
      msc_active = true;
      MSC.mediaPresent(true);
    }
  }

  // Pause SD Share
  // Stops share but MSC still acrive.
  void MSC_Share::msc_pause() {
    if (msc_started && msc_active) {
      log_d("msc_pause");
      MSC.mediaPresent(false);
      msc_active = false;
      // Re-init SD after host may have written
      // sd_obj.initSD();
    }
  }

  void MSC_Share::RunSetup() {
    extern SDInterface sd_obj;
      delay(500);

    if (!sd_obj.supported) {
      log_d("MSC_Share:  SD not avalible");
      return;
    }

    #if defined(HAS_SDMMC) && defined(USE_MMC_WRITE_SECTORS)
      if (!_mmc_card) {
        log_w("MSC_Share: no card handle, will use writeRAW fallback");
      } else {
        log_d("MSC_Share: using sdmmc_write_sectors");
      }
    #endif

    log_d("MSC_Share: numSectors=%u sectorSize=%u\n",
    SD.numSectors(), SD.sectorSize());

    MSC.vendorID("ESP32 Marauder");
    MSC.productID(HARDWARE_NAME);
    MSC.productRevision(MARAUDER_VERSION);
    MSC.onRead(onRead);
    MSC.onWrite(onWrite);
    MSC.onStartStop(onStartStop);
    MSC.mediaPresent(true);

    #if ARDUINO_USB_CDC_ON_BOOT
      // USB already started at boot just begin MSC
      MSC.begin(SD.numSectors(), SD.sectorSize());
    #else
      // USB not yet started begin MSC first, then start USB
      MSC.begin(SD.numSectors(), SD.sectorSize());
      USB.begin();
    #endif

    msc_active = true;
    msc_started = true;
    log_d("MSC_Share: RunSetup Complete");
    delay(5);
  }

  // stop MSC system
  void MSC_Share::ShareEnd() {
      delay(10);
      msc_active = false;
      msc_started = false;
      MSC.mediaPresent(false);
      MSC.end();
      log_d("MSC_Share::ShareEnd Complete");
      delay(100);   // Watchdog
      // sd_obj.initSD();
  }
#endif  // MSC_SHARE


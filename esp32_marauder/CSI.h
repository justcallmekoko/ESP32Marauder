#pragma once

#ifndef CSI_H
#define CSI_H

#include "configs.h"

#ifdef HAS_CSI

#include <Arduino.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_timer.h"

#define CSI_STREAM_IDLE   0
#define CSI_STREAM_ACTIVE 1

#define CSI_SYNC0 0xAB
#define CSI_SYNC1 0xCD

#define CSI_MAX_SC 64
#define CSI_EMA_SHIFT 5
#define CSI_VAR_SHIFT 4
#define CSI_MOTION_THRESH 800

class CsiModule {
  private:
    uint8_t  streamState;
    uint32_t packetCount;
    uint32_t lastPacketTime;
    uint32_t lastDisplayUpdate;
    uint32_t lastGraphUpdate;
    uint32_t lastMotionUpdate;
    bool     wifiOwned;
    uint8_t  operatingChannel;
    int8_t   peakRssi;
    bool     returnToMenu;
    uint8_t  viewMode;
    bool     personDetected;
    uint32_t motionScore;
    uint32_t totalEnergy;

    uint16_t subMag[CSI_MAX_SC];
    int16_t  subEma[CSI_MAX_SC];
    uint16_t subDev[CSI_MAX_SC];
    uint8_t  numSub;

    void takeWiFi();
    void releaseWiFi();
    static void csiDataCallback(void *ctx, wifi_csi_info_t *data);
    void processCsiData(wifi_csi_info_t *data);
    void sendCsiBinary(wifi_csi_info_t *data);
    void computeMag(wifi_csi_info_t *data);
    void updateMotion();
    void drawHeader();
    void drawGraph();
    void drawStats();
    void drawHints();

  public:
    CsiModule();
    void setup();
    void start(uint8_t channel = 1);
    void stop();
    void main(uint32_t currentTime);
    bool isActive() { return streamState == CSI_STREAM_ACTIVE; }
    bool shouldReturnToMenu() { return returnToMenu; }
    void clearReturnFlag() { returnToMenu = false; }
};

#endif // HAS_CSI
#endif

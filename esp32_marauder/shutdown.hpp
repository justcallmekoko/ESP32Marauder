
#pragma once

#ifndef SHUTDOWN_HPP
#define SHUTDOWN_HPP

#if defined(POWER_HOLD_PIN) || (defined(HAS_BATTERY) && (defined(HAS_AXP2101) || defined(HAS_AXP192))) || defined(DEEPSLEEP)

  #if defined(ES8311) && defined(HAS_SOUND)
    #include "Sound.hpp"
  #endif

  #if defined(HAS_BATTERY) && (defined(HAS_AXP2101) || defined(HAS_AXP192))
    #include "BatteryInterface.h"
    extern BatteryInterface battery_obj;
  #endif

  // should this be in a separate .cpp file
  static void DeepSleep(int8_t wakeup_but = -1) {

    // 1. Disconnect from the network gracefully
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();

    #ifdef HAS_BT
      // This handles stopping and deinitializing BT gracefully
      // esp_bluedroid_disable();
      esp_bt_controller_disable();
      esp_bt_controller_deinit();
    #endif

    #if defined(HAS_CH32V00X)
      CH32V00X_obj.SetAudio(0);
      CH32V00X_obj.setPWM(0);
      log_d("CH32V00X Powerdown");
    #endif
      delay(200);

    #if defined(ES8311) && defined(HAS_SOUND)
      sound_obj.PowerOff();
      log_d("ES8311 PowerOff");
    #endif
      

    #if defined(HAS_SCREEN)  && defined(ST7789_DRIVER)

      // 1. Put the display driver into sleep mode
      display_obj.tft.writecommand(0x10); // Sleep-in command (SLPIN)
      delay(5);               // Wait for shutdown time

      // 2. Turn off the LCD display output (if using ST7789 or similar)
      display_obj.tft.writecommand(0x38); // Or use ST7789_DISPOFF

      // 3. Turn off the backlight (replace 38 with your actual backlight GPIO pin)

      #if defined(TFT_BL) && !defined(HAS_CH32V00X)
        digitalWrite(TFT_BL, LOW);  
      #endif

    #endif

    // Should we isolate  pins with external pull-up resistors
    // to minimize current consumption.
    // #ifdef I2C_SDA
    //   rtc_gpio_isolate(I2C_SDA);
    //   rtc_gpio_isolate(I2C_SCL);
    // #endif

    // Code specific to the classic ESP32 (e.g., WROOM-32) goes here
    // #ifdef CONFIG_IDF_TARGET_ESP32
    // rtc_gpio_isolate(GPIO_NUM_12);
    // 18 19 5 23 10 33 32 16 17 20 
    esp_sleep_config_gpio_isolate();
    
    if (wakeup_but >= 0) {
      gpio_hold_dis((gpio_num_t) wakeup_but);
      pinMode(wakeup_but, INPUT_PULLUP);

    // Configure the wake-up source: wake up when GPIO 0 goes LOW (button press)
    #if SOC_PM_SUPPORT_EXT_WAKEUP
	// For classic ESP32 which supports EXT0 (e.g., ESP32)
        esp_sleep_enable_ext0_wakeup((gpio_num_t)wakeup_but, 0); // 0 means LOW
    #elif SOC_PM_SUPPORT_GPIO_WAKEUP
       // For newer chips that use generic GPIO wakeup (e.g., ESP32-C3, ESP32-S3)
      esp_deep_sleep_enable_gpio_wakeup((1ULL << wakeup_but), ESP_GPIO_WAKEUP_GPIO_LOW);
    #else
      #warning "Unsupported sleep/wakeup architecture on this chip"
    #endif


    }

    Serial.println("Going to sleep now...");
    Serial.flush();
    delay(100); // Give serial monitor time to flush

    // Enter deep sleep
    esp_deep_sleep_start();
  }

  static void shutdown_system() {

    #ifdef POWER_HOLD_PIN
        // T-HMI
        //  if on battery, can be turn off with the PWR_ON_PIN/POWER_HOLD_PIN if on battery
        Serial.println("Set POWER_HOLD_PIN:  LOW");
        Serial.flush();
        digitalWrite(POWER_HOLD_PIN, LOW);

    #elif defined(HAS_BATTERY) && (defined(HAS_AXP2101) || defined(HAS_AXP192))
        log_d("battery_obj PowerOff");
        Serial.println("battery_obj PowerOff");
        Serial.flush();
        battery_obj.PowerOff();

    #endif
        //  if plugged in or Powerdown failed use DEEPSLEEP instead
        delay(500);
        Serial.println("DeepSleep");
        DeepSleep();
  }

#endif  // DEEPSLEEP / POWER_HOLD_PIN
#endif  // SHUTDOWN_HPP

#ifdef NEVER_EVER
/*

CST3530 capacitive touch:
    • Hardware Deep Sleep: If your host MCU is going into a deep
    sleep, hold the CST3530 RESET pin LOW to lock the chip into its
    lowest hardware reset state.

CST820 (CST816 Family)
    write to the control registers is required to change this state.
    Writing 0xFF to register 0xFE is commonly used to disable the
    aggressive auto-low-power behavior during active MCU cycles.

FocalTech FT6336
    FT6336 into its lowest sub-microamp Hibernation state, write 0x03
    to the Device Mode Register (0x00).

XPT2046 (Resistive, SPI)
    • Every SPI command byte sent to the XPT2046 contains two
    dedicated Power-Down bits (PD1 and PD0, which are bits 1 and 0
    of the control byte). To force the chip into an absolute
    ultra-low-power sleep state at the end of a transaction, you
    must intentionally set these bits to 00 on the very last byte
    of your SPI communication:

    PD1 Bit Value   PD0 Bit Value   Power State Result

         0           0      Power-Down Mode (Sleep). Alternating conversion drivers are
                            disabled, and the PENIRQ (Pen Interrupt) output is safely
                            activated.


ST7789
      TFT Code to Wake Up and Power On

      // 1. Turn the backlight back on
      digitalWrite(38, HIGH); 

      // 2. Wake the display driver up
      tft.writecommand(0x11); // Sleep-out command (SLPOUT)
      delay(120);             // Wait for power supplies to stabilize

      // 3. Reinitialize or turn display on if needed
      tft.writecommand(0x29); // Display-on command (DISPON) - or
                              reinit if required by your board


ES8311
  • Software Power-Down Control: Power management is split across
    multiple registers. To place the ES8311 into a deep power-down state
    via I2C (Address: 0x18), you must disable the internal digital and
    analog subsystems sequentially to avoid speaker pops:

      1. Register 0x19 (ADC/DAC Control): Power down the digital filters.
      2. Register 0x1C (Power Management): Power down the analog biases.
      3. Register 0x00 (System Control): Set the chip into standby.

  • Low-Power Register Toggles: If you want to conserve power during
    active audio streaming without turning the device completely off,
    you can use the Low Power Control Register (0x0F):

      • Set LPDAC (Bit 7) = 1 to activate low-power mode for the DAC.
      • Set LPPGA (Bit 6) = 1 to activate low-power mode for the PGA input.

  • Hardware Shutdown Limitation: Unlike other complex codecs, the
    ES8311 does not feature a dedicated hardware power-down pin. It
    depends on an external GPIO-controlled Audio PA Enable/Mute switch
    (routing to your amplifier) to handle physical audio isolation,
    while the chip itself is managed strictly over the I2C configuration
    registers.

*/
#endif


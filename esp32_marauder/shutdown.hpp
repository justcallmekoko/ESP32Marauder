
#pragma once

#ifndef SHUTDOWN_HPP
#define SHUTDOWN_HPP

#if defined(DEEPSLEEP) || defined(POWER_HOLD_PIN)

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

  static void shutdown() {
    #ifdef POWER_HOLD_PIN
        // T-HMI
        //  if on battery, can be turn off with the PWR_ON_PIN/POWER_HOLD_PIN if on battery
        Serial.println("Set POWER_HOLD_PIN:  LOW");
        Serial.flush();
        digitalWrite(POWER_HOLD_PIN, LOW);

        //  if plugged in we use DEEPSLEEP instead
        delay(500);
        Serial.println("DeepSleep");
        DeepSleep();
    #else
        DeepSleep(0);
    #endif
  }

#endif  // DEEPSLEEP / POWER_HOLD_PIN
#endif  // SHUTDOWN_HPP

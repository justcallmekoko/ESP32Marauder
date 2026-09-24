#pragma once

#include "configs.h"
// ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |

#ifndef __cpu_temp_sensor_hpp__
#define __cpu_temp_sensor_hpp__ 1

  #if (defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR > 5)) || \
        !(defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3) \
        || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5) \
        || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2) )
     #warning CPU temperature_sensor not supported
     #undef HAS_CPU_TEMP
  #endif

#if defined(HAS_CPU_TEMP)


#include "driver/temperature_sensor.h"

inline temperature_sensor_handle_t temp_handle = NULL;
inline float _celsius = 0.0;

inline bool init_sys_temp() {
esp_err_t x = 0;

    log_d("init_sys_temp");

    // Configure sensor range (e.g., -10°C to 80°C)
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);

    // Install and enable the internal sensor
    x = temperature_sensor_install(&temp_sensor_config, &temp_handle);
    ESP_ERROR_CHECK(x);

    log_d("temperature_sensor_install: x = %d", x);
    if (x != ESP_OK) return false;

    x = temperature_sensor_enable(temp_handle);
    ESP_ERROR_CHECK(x);

    log_d("temperature_sensor_enable: x = %d", x);
    log_d("x = %d", x);
    return x == ESP_OK;
  }

  inline float get_sys_temperature() { return _celsius; }

  inline float read_sys_temp() {
    // Read the temperature in Celsius
    if (temperature_sensor_get_celsius(temp_handle, &_celsius) == ESP_OK) {
      log_d("Chip Temperature: %f", _celsius);
      return _celsius;
    } else {
      log_d("Error reading temperature");

      return 0.0;
    }

    return _celsius;
  }

  inline void disable_sys_temp() {
    temperature_sensor_disable(temp_handle);
  }


  #else   // HAS_CPU_TEMP

    // this should not happen
    inline bool init_sys_temp() { return false }
  inline float get_sys_temp() { return 0.0 }
  inline float temperature() { return 0.0 }

#endif   // HAS_CPU_TEMP & CONFIG_IDF_TARGET

#endif   // __cpu_temp_sensor_hpp__



// temperatureRead  for older Hardware,   use not recommended
#ifdef NODEF
    // Legacy reading for original ESP32
    extern uint8_t temprature_sens_read(void);

    void read_chip_temp(void) {
        // Converts raw register values from Fahrenheit to Celsius
        float tsens_out = (temprature_sens_read() - 32) / 1.8;
        Serial.printf("Internal Temperature (Legacy): %.2f °C\n", tsens_out);
#endif   // NODEF

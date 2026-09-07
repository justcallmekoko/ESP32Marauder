
#include "configs.h"

#ifndef cpu_temp_sensor_hpp
#define cpu_temp_sensor_hpp 1

#if defined(USE_CPU_TEMP) && \
    ( defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5) \
    || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32S3) )

// ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |


#include "driver/temperature_sensor.h"

inline temperature_sensor_handle_t temp_handle = NULL;

inline void init_sys_temp() {
  // Configure sensor range (e.g., -10°C to 80°C)
  temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
  
  // Install and enable the internal sensor
  ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_handle));
  ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));
}

inline float get_sys_temp() {
  float celsius = 0.0;

  // Read the temperature in Celsius
  if (temperature_sensor_get_celsius(temp_handle, &celsius) == ESP_OK) {
    // log_d("Chip Temperature: %f", celsius);
  } else {
    log_d("Error reading temperature");
    
    return 0.0;
  }
  
  return celsius;
}

inline void disable_sys_temp() {
  temperature_sensor_disable(temp_handle);
}


#endif   // USE_CPU_TEMP & CONFIG_IDF_TARGET
#endif   // cpu_temp_sensor_hpp


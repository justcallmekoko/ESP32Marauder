#include "Sound_CYD.h"

#ifdef CYD_SOUND

#define LEDC_RESOLUTION 10  // Set resolution to 10 bits
#define DUTY 512

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    #define LEDC_CHANNEL SOUND_PIN
    #define LEDCDETACH  ledcDetach
#else
    #define LEDC_CHANNEL 1
    #define LEDCDETACH  ledcDetachPin
#endif

void Sound_CYD::RunSetup() {
  // Serial.println("Sound_CYD::RunSetup");

  duty_cycle = DUTY;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(SOUND_PIN, 1, LEDC_RESOLUTION);
#else
  ledcAttachPin(SOUND_PIN, LEDC_CHANNEL);
  ledcSetup(LEDC_CHANNEL, 1, LEDC_RESOLUTION);
#endif
  ledcWrite(LEDC_CHANNEL, duty_cycle);
  ledcWriteTone(SOUND_PIN, 0);
}


void Sound_CYD::tic() {
    // # Serial.println("tic");
    if (!settings_obj.loadSetting<bool>("EnableSND")) {
        return;
    }

    ledcWriteTone(LEDC_CHANNEL, 150);
    delay(20);
    ledcWriteTone(LEDC_CHANNEL, 180);
    delay(10);

    ledcWriteTone(LEDC_CHANNEL, 0);
}

void Sound_CYD::tok() {
    // Serial.println("tok");
    if (!settings_obj.loadSetting<bool>("EnableSND")) {
        return;
    }

    ledcWriteTone(LEDC_CHANNEL, 150);
    delay(20);
    ledcWriteTone(LEDC_CHANNEL, 110);
    delay(10);

    ledcWriteTone(LEDC_CHANNEL, 0);
}

void Sound_CYD::click() {
    if (!settings_obj.loadSetting<bool>("EnableSND")) {
        return;
    }
    doit(400, 8);
}

void Sound_CYD::doit(double s,  int t) {
    // Serial.println("ledcWriteTone :" + (String)s);
    ledcWriteTone(LEDC_CHANNEL, s);
    delay(t);

    ledcWriteTone(LEDC_CHANNEL, 0);
}


void Sound_CYD::alert() {
    if (!settings_obj.loadSetting<bool>("EnableSND")) {
        return;
    }

    for (int i=1; i < 14; i++) {
        ledcWriteTone(LEDC_CHANNEL, i * 100);
        delay(4);
      }

    ledcWriteTone(LEDC_CHANNEL, 0);
}

void Sound_CYD::geigerClick(uint8_t i) {
    if (!settings_obj.loadSetting<bool>("EnableSND")) {
        return;
    }

    if (i) {
        doit(250, 10);
    } else {
        doit(150, 10);
    }
}
#endif

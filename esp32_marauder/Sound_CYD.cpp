#include "Sound_CYD.h"

#ifdef CYD_SOUND

#define LEDC_RESOLUTION 8  // Set resolution to 10 bits
#define DUTY (std::pow(2, LEDC_RESOLUTION)/2 -1) 
#ifndef SND_CHANNEL
  #define SND_CHANNEL 3
#endif

void Sound_CYD::RunSetup() {
  // Serial.println("Sound_CYD::RunSetup");

  duty_cycle = DUTY;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // ledcAttach(SOUND_PIN, 1, LEDC_RESOLUTION);
  ledcAttachChannel(SOUND_PIN, 5000, LEDC_RESOLUTION, SND_CHANNEL);
  #define SND_ID  SOUND_PIN
#else
  ledcAttachPin(SOUND_PIN, SND_CHANNEL);
  ledcSetup(SND_CHANNEL, 5000, LEDC_RESOLUTION);
  #define SND_ID SND_CHANNEL
  #define ledcDetach  ledcDetachPin
#endif

  // ledcWrite(SND_CHANNEL, duty_cycle);
  ledcWriteTone(SND_ID, 0);
}

void Sound_CYD::gen_tone(uint32_t f, uint32_t t) {
    // Serial.println("ledcWriteTone :" + (String)s);
    ledcWriteTone(SND_ID, f);
    delay(t);

    ledcWriteTone(SND_ID, 0);
    delay(12);
}

void Sound_CYD::stop_tone() {
    ledcWriteTone(SND_ID, 0);
}

void Sound_CYD::s_power_on() { gen_tone(523,80); gen_tone(659,100); gen_tone(784,120); }
void Sound_CYD::s_beep() { gen_tone(1000,80); }
void Sound_CYD::s_ready() { gen_tone(523,100); gen_tone(659,100); gen_tone(784,100); gen_tone(1046,180); }
void Sound_CYD::s_ready_2() { gen_tone(784,80); delay(50); gen_tone(1046,100); }            // Ready


void Sound_CYD::s_error() { gen_tone(523,150); delay(50); gen_tone(330,180); }            // Error
void Sound_CYD::s_error_2() { for(int i=0;i<3;i++){ gen_tone(300,150); delay(50);} }


void Sound_CYD::tic() {
    // # Serial.println("tic");
    //if (!settings_obj.loadSetting<bool>("EnableSND")) {
    //    return;
    //}

    gen_tone(150, 20);
    gen_tone(180, 10);

    /*
    ledcWriteTone(SND_ID, 150);
    delay(20);
    ledcWriteTone(SND_ID, 180);
    delay(10);
    ledcWriteTone(SND_ID, 0);
    */
}

void Sound_CYD::tok() {
    // Serial.println("tok");
    //if (!settings_obj.loadSetting<bool>("EnableSND")) {
    //    return;
    //}

    gen_tone(150, 20);
    gen_tone(110, 10);

    /*
    ledcWriteTone(SND_ID, 150);
    delay(20);
    ledcWriteTone(SND_ID, 110);
    delay(10);
    ledcWriteTone(SND_ID, 0);
    */
}

void Sound_CYD::click() {
    //if (!settings_obj.loadSetting<bool>("EnableSND")) {
    //    return;
    //}
    gen_tone(400, 8);
}


void Sound_CYD::geigerClick(uint8_t i) {
    //if (!settings_obj.loadSetting<bool>("EnableSND")) {
    //    return;
    //}

    if (i) {
        gen_tone(250, 10);
    } else {
        gen_tone(150, 10);
    }
}

#endif

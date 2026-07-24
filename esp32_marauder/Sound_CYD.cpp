#include "Sound_CYD.h"

#ifdef CYD_SOUND

#ifndef SND_CHANNEL
  #define SND_CHANNEL 3
#endif

void Sound_CYD::RunSetup() {
  log_i("Sound_CYD::RunSetup: SOUND_PIN=%d CHANNEL=%d", SOUND_PIN, LEDC_RESOLUTION, SND_CHANNEL);
  // duty_cycle = DUTY;


  #ifndef DEVELOPER
    Serial.println("SOUND_PIN :" + (String)SOUND_PIN);
  #else
    log_d("SND_CHANNEL : %d",  SND_CHANNEL);
    log_i("SOUND_PIN : %d",  SOUND_PIN);
  #endif
  // ledcWrite(SND_CHANNEL, duty_cycle);
  setToneChannel(SND_CHANNEL);
  noTone(SOUND_PIN);
}



// void Sound_CYD::s_power_on() { Tone(SOUND_PIN,523,80); Tone(SOUND_PIN,659,100); Tone(SOUND_PIN,784,120); }
// void Sound_CYD::s_beep() { Tone(SOUND_PIN,1000,80); }
// void Sound_CYD::s_ready() { Tone(SOUND_PIN,523,100); Tone(SOUND_PIN,659,100); Tone(SOUND_PIN,784,100); Tone(SOUND_PIN,1046,180); }
// void Sound_CYD::s_ready_2() { Tone(SOUND_PIN,784,80); delay(50); Tone(SOUND_PIN,1046,100); }            // Ready


// void Sound_CYD::s_error() { Tone(SOUND_PIN,523,150); delay(50); Tone(SOUND_PIN,330,180); }            // Error
// void Sound_CYD::s_error_2() { for(int i=0;i<3;i++){ Tone(SOUND_PIN,300,150); delay(50);} }


/*
void Sound_CYD::tic() {
    // # Serial.println("tic");
    if (!settings_obj.loadSetting<bool>("EnableSND")) {
        return;
    }

    Tone(SOUND_PIN,150, 20);
    Tone(SOUND_PIN,180, 10);
}

void Sound_CYD::tok() {
    // Serial.println("tok");
    if (!settings_obj.loadSetting<bool>("EnableSND")) { return; }

    Tone(SOUND_PIN,150, 20);
    Tone(SOUND_PIN,110, 10);
}
*/

void Sound_CYD::click() {
    if (!settings_obj.loadSetting<bool>("EnableSND")) { return; }
    tone(SOUND_PIN, 400, 8);
}


void Sound_CYD::geigerClick(uint8_t i) {
    if (!settings_obj.loadSetting<bool>("EnableSND")) { return; }

    //Serial.println("Sound_CYD::geigerClick");
    if (i) {
        tone(SOUND_PIN, 250, 10);
    } else {
        tone(SOUND_PIN, 150, 10);
    }
}

#endif

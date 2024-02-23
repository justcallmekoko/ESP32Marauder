#include "cydLED.h"

// RGB LED INFO
//  Active low control
//    LOW = On
//    HIGH = OFF
//  Pinout
//    IO4   Red
//    IO16  Green
//    IO17  Blue

void cydLED::RunSetup() {
  pinMode(B_PIN, OUTPUT);
  pinMode(G_PIN, OUTPUT);
  pinMode(R_PIN, OUTPUT);

  if (!settings_obj.loadSetting<bool>("EnableLED")) {
    digitalWrite(B_PIN, LOW);
    digitalWrite(G_PIN, LOW);
    digitalWrite(R_PIN, LOW);
    return;
  }
    
  delay(50);

  digitalWrite(B_PIN, LOW);
  delay(500);
  digitalWrite(B_PIN, HIGH);
  digitalWrite(G_PIN, LOW);
  delay(500);
  digitalWrite(G_PIN, HIGH);
  digitalWrite(R_PIN, LOW);
  delay(500);
  digitalWrite(R_PIN, HIGH);
}

void cydLED::attackLED() {
  if (!settings_obj.loadSetting<bool>("EnableLED"))
    return;
    
  digitalWrite(B_PIN, HIGH);
  digitalWrite(G_PIN, HIGH);
  digitalWrite(R_PIN, HIGH); 
  delay(10);
  digitalWrite(R_PIN, LOW);
}

void cydLED::sniffLED() {
  if (!settings_obj.loadSetting<bool>("EnableLED"))
    return;
    
  digitalWrite(B_PIN, HIGH);
  digitalWrite(G_PIN, HIGH);
  digitalWrite(R_PIN, HIGH);
  delay(10);
  digitalWrite(B_PIN, LOW);
}

void cydLED::offLED() {
  if (!settings_obj.loadSetting<bool>("EnableLED"))
    return;
    
  digitalWrite(B_PIN, HIGH);
  digitalWrite(G_PIN, HIGH);
  digitalWrite(R_PIN, HIGH);
}

void cydLED::main() {
  // do nothing
}

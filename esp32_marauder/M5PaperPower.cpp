#include "configs.h"
#ifdef MARAUDER_M5PAPER
#include "M5PaperPower.h"
#include "Display.h"
#include "ReconMission.h"
#include "WiFiScan.h"
#include "Buffer.h"
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <SD.h>
#include <SPIFFS.h>

extern Display display_obj;
extern ReconMission recon_obj;
extern WiFiScan wifi_scan_obj;
extern Buffer buffer_obj;

namespace m5paper {
namespace {
sticky::PowerButton powerButton;
sticky::DebouncedButton upButton, downButton;
bool enabled = false, shuttingDown = false, selectPending = false;
uint32_t selectedAt = 0;
void holdLow(int pin) {
  gpio_hold_dis(gpio_num_t(pin));
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  gpio_hold_en(gpio_num_t(pin));
}
}

void beginButtons() {
  for (int pin : {C_BTN, U_BTN, D_BTN}) pinMode(pin, INPUT);
  enabled = true;
  Serial.printf("M5Paper power: wake cause=%d, buttons UP=37 DOWN=39 POWER=38\n",
                int(esp_sleep_get_wakeup_cause()));
}

void servicePowerButton() {
  if (!enabled || shuttingDown) return;
  const uint32_t now = millis();
  const auto event = powerButton.update(digitalRead(C_BTN) == LOW, now);
  if (event == sticky::PowerEvent::Shutdown) powerOff();
  else if (event == sticky::PowerEvent::Select) {
    selectPending = true;
    selectedAt = now;
    Serial.println("M5Paper button: POWER short release");
  }
}

bool takePowerSelect() {
  servicePowerButton();
  const bool selected = selectPending && uint32_t(millis() - selectedAt) < 250;
  selectPending = false;
  return selected;
}

bool navigationDown(int pin) {
  auto& button = pin == U_BTN ? upButton : downButton;
  if (button.update(digitalRead(pin) == LOW, millis()))
    Serial.printf("M5Paper button: %s %s\n", pin == U_BTN ? "UP" : "DOWN",
                  button.down ? "pressed" : "released");
  return button.down;
}

void powerOff(uint32_t wakeAfterMs) {
  if (shuttingDown) return;
  shuttingDown = true;
  selectPending = false;
  Serial.println("M5Paper: shutting down; stopping activity and saving buffer");
  recon_obj.stop();
  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  WiFi.mode(WIFI_OFF);
  buffer_obj.save();
  display_obj.tft.showPowerOff();
  SD.end();
  SPIFFS.end();

  // The USB supply bypasses the main-power MOS. Deep sleep covers USB too.
  for (int pin : {5, 23}) holdLow(pin);
  uint32_t releasedAt = millis();
  while (uint32_t(millis() - releasedAt) < 60) {
    if (digitalRead(C_BTN) == LOW) releasedAt = millis();
    delay(5);
  }
  Serial.println("M5Paper: main power off; USB fallback deep sleep; GPIO38 wake");
  Serial.flush();
  holdLow(2);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(GPIO_NUM_38, 0));
  if (wakeAfterMs) ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(uint64_t(wakeAfterMs) * 1000));
  gpio_deep_sleep_hold_en();
  esp_deep_sleep_start();
}
}
#endif

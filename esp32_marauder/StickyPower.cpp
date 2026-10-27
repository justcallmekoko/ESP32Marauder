#include "configs.h"
#ifdef MARAUDER_RETERMINAL_STICKY
#include "StickyPower.h"
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

namespace sticky {
namespace {
PowerButton powerButton;
DebouncedButton upButton, downButton;
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
  for (int pin : {C_BTN, U_BTN, D_BTN}) pinMode(pin, INPUT_PULLUP);
  enabled = true;
  Serial.printf("Sticky power: wake cause=%d, buttons UP=5 DOWN=6 POWER=4\n",
                int(esp_sleep_get_wakeup_cause()));
}

void servicePowerButton() {
  if (!enabled || shuttingDown) return;
  const uint32_t now = millis();
  const auto event = powerButton.update(digitalRead(C_BTN) == LOW, now);
  if (event == PowerEvent::Shutdown) powerOff();
  else if (event == PowerEvent::Select) {
    selectPending = true;
    selectedAt = now;
    Serial.println("Sticky button: POWER short release");
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
    Serial.printf("Sticky button: %s %s\n", pin == U_BTN ? "UP" : "DOWN",
                  button.down ? "pressed" : "released");
  return button.down;
}

void powerOff(uint32_t wakeAfterMs) {
  if (shuttingDown) return;
  shuttingDown = true;
  selectPending = false;
  Serial.println("Sticky: shutting down; stopping activity and saving buffer");
  recon_obj.stop();
  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  WiFi.mode(WIFI_OFF);
  buffer_obj.save();
  display_obj.tft.showPowerOff();
  SD.end();
  SPIFFS.end();

  // Seeed's power tree has a USB bypass. Release the battery latch and
  // enter deep sleep as well, so plugging in USB does not keep Marauder running.
  for (int pin : {47, 42, 41, 38, 10, 48}) holdLow(pin);
  uint32_t releasedAt = millis();
  while (uint32_t(millis() - releasedAt) < 60) {
    if (digitalRead(C_BTN) == LOW) releasedAt = millis();
    delay(5);
  }
  // Schematic U3 is a positive-edge D flip-flop: D=GPIO45, CP=GPIO46.
  // Merely lowering both pins does not clear the latched power output.
  digitalWrite(46, LOW);
  digitalWrite(45, LOW);
  delayMicroseconds(10);
  Serial.println("Sticky: power latch off; USB fallback deep sleep; GPIO4 wake");
  Serial.flush();
  digitalWrite(46, HIGH);
  delay(20);
  // If USB supplies the bypass rail, execution reaches this point.
  gpio_hold_en(GPIO_NUM_45);
  gpio_hold_en(GPIO_NUM_46);
  pinMode(C_BTN, INPUT_PULLUP);
  gpio_hold_en(GPIO_NUM_4);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(1ULL << C_BTN, ESP_EXT1_WAKEUP_ANY_LOW));
  // Only the explicit USB diagnostic requests a timed restart.
  if (wakeAfterMs) ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(uint64_t(wakeAfterMs) * 1000));
  gpio_deep_sleep_hold_en();
  Serial.println("Sticky: USB power present; entering deep sleep");
  Serial.flush();
  esp_deep_sleep_start();
}
}
#endif

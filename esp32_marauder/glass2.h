#pragma once
// Glass2 Grove secondary OLED (SSD1309, 128x64) — Cardputer ADV only.
// Grove HY2.0-4P port: GPIO2=SDA, GPIO1=SCL.
// Uses M5UnitGLASS2 from M5GFX (transitive via M5Unified — no extra lib_deps needed).
// Include from exactly one translation unit (esp32_marauder.ino).
#include <M5UnitGLASS2.h>

static M5UnitGLASS2 _g2(/*sda=*/2, /*scl=*/1);
static bool _g2_ready = false;

inline void glass2Init() {
    _g2_ready = _g2.init();
    if (_g2_ready) {
        _g2.fillScreen(TFT_BLACK);
        _g2.setTextColor(TFT_WHITE, TFT_BLACK);
    }
}

inline void glass2Show(const char* l1, const char* l2 = nullptr,
                       const char* l3 = nullptr, const char* l4 = nullptr) {
    if (!_g2_ready) return;
    _g2.fillScreen(TFT_BLACK);
    _g2.setTextSize(1);
    _g2.setTextColor(TFT_WHITE, TFT_BLACK);
    if (l1) { _g2.setCursor(0,  0); _g2.print(l1); }
    if (l2) { _g2.setCursor(0, 16); _g2.print(l2); }
    if (l3) { _g2.setCursor(0, 32); _g2.print(l3); }
    if (l4) { _g2.setCursor(0, 48); _g2.print(l4); }
}

inline void glass2Clear() {
    if (!_g2_ready) return;
    _g2.fillScreen(TFT_BLACK);
}

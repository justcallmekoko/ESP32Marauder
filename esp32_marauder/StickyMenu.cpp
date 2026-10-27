#include "configs.h"
#ifdef MARAUDER_EPAPER_UI
#include "MenuFunctions.h"
#include "lang_var.h"
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

extern const unsigned char menu_icons[][66];

namespace {
String fitLabel(TFT_eSPI& screen, String text, int width) {
  if (screen.textWidth(text) <= width) return text;
  while (text.length() && screen.textWidth(text + "...") > width)
    text.remove(text.length() - 1);
  return text + "...";
}
void restoreText(TFT_eSPI& screen) {
  screen.setFreeFont(nullptr);
  screen.setTextSize(2);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
}
}

void MenuFunctions::drawStickyMenuRow(int index, bool focused) {
  if (!current_menu || !current_menu->list || index < menu_start_index ||
      index >= current_menu->list->size() || index >= menu_start_index + BUTTON_SCREEN_LIMIT) return;
  auto& screen = display_obj.tft;
  const auto node = current_menu->list->get(index);
  const int center = KEY_Y + (index - menu_start_index) * (KEY_H + KEY_SPACING_Y);
  const int top = center - KEY_H / 2;
  const uint16_t paper = TFT_BLACK;
  const uint16_t ink = TFT_WHITE;
  screen.fillRect(16, top, KEY_W, KEY_H, TFT_BLACK);
  // Only this marker changes when moving the selection. Text stays stable.
  if (focused) screen.fillTriangle(17, center - 8, 17, center + 8, 25, center, ink);
  screen.drawFastHLine(32, top + KEY_H - 1, KEY_W - 28, TFT_WHITE);
  screen.setTextColor(ink, paper);
  screen.setTextSize(1);
  screen.setFreeFont(&FreeSans12pt7b);
  const bool setting = node.icon == SETTINGS && node.color == TFTLIGHTGREY;
  const bool marked = setting || node.selected;
  const String label = node.name == text09 ? String("Back") : node.name;
  screen.drawString(fitLabel(screen, label, screen.width() - (marked ? 152 : 134)), 68, center + 8);
  if (node.name == text09) {
    screen.drawLine(34, center, 50, center, ink);
    screen.drawLine(34, center, 41, center - 7, ink);
    screen.drawLine(34, center, 41, center + 7, ink);
  } else if (node.icon != 255) {
    screen.drawXBitmap(32, center - ICON_H / 2, menu_icons[node.icon], ICON_W, ICON_H, paper, ink);
  }
  if (marked) {
    screen.drawRoundRect(screen.width() - 54, center - 10, 20, 20, 3, ink);
    if (node.selected) {
      screen.drawLine(screen.width() - 50, center, screen.width() - 45, center + 5, ink);
      screen.drawLine(screen.width() - 45, center + 5, screen.width() - 37, center - 5, ink);
    }
  }
  restoreText(screen);
}

void MenuFunctions::drawStickyMenuFooter() {
  if (!current_menu || !current_menu->list) return;
  auto& screen = display_obj.tft;
  const int footer = screen.height() - 84;
  screen.fillRect(16, footer, screen.width() - 32, 84, TFT_BLACK);
  restoreText(screen);
  const int count = current_menu->list->size();
  const String position = "PAGE " + String(menu_start_index / BUTTON_SCREEN_LIMIT + 1) + " / " +
      String(max(1, (count + BUTTON_SCREEN_LIMIT - 1) / BUTTON_SCREEN_LIMIT));
  screen.drawCentreString(position, screen.width() / 2, footer + 8);
  screen.drawFastHLine(24, footer + 36, screen.width() - 48, TFT_WHITE);
  screen.setTextSize(1);
  screen.drawCentreString("TAP TOP", screen.width() / 6, footer + 50);
  screen.drawCentreString("TAP MIDDLE", screen.width() / 2, footer + 50);
  screen.drawCentreString("TAP BOTTOM", screen.width() * 5 / 6, footer + 50);
  screen.setTextSize(2);
  screen.drawCentreString("UP", screen.width() / 6, footer + 64);
  screen.drawCentreString("SELECT", screen.width() / 2, footer + 64);
  screen.drawCentreString("DOWN", screen.width() * 5 / 6, footer + 64);
}

void MenuFunctions::drawStickyMenu() {
  auto& screen = display_obj.tft;
  screen.setRotation(SCREEN_ORIENTATION);
  display_obj.clearScreen();
  drawStatusBar();
  screen.setTextWrap(false);
  screen.setTextSize(1);
  screen.setFreeFont(&FreeSansBold18pt7b);
  screen.setTextColor(TFT_WHITE, TFT_BLACK);
  const String title = current_menu == &mainMenu ? String("Main menu") : current_menu->name;
  screen.drawString(fitLabel(screen, title, screen.width() - 48), 24, 78);
  screen.drawFastHLine(24, 100, screen.width() - 48, TFT_WHITE);
  if (current_menu->list)
    for (int i = menu_start_index; i < min(menu_start_index + BUTTON_SCREEN_LIMIT, current_menu->list->size()); ++i)
      drawStickyMenuRow(i, current_menu->selected == i);
  drawStickyMenuFooter();
  restoreText(screen);
}
#endif

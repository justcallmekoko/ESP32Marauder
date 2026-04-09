#pragma once
#ifndef T9Keyboard_h
#define T9Keyboard_h

#include "configs.h"
#include "Display.h"

#ifdef HAS_OLED

// Screen: 128x64
// Layout: input bar (10px) | key grid (54px across 4 rows)
#define T9_NUM_KEYS      12
#define T9_KEYS_PER_ROW   3
#define T9_ROWS           4
#define T9_KEY_WIDTH     42   // 3 * 42 = 126, leaves 2px margin
#define T9_KEY_HEIGHT    13   // 4 * 13 = 52, grid ends at y=63
#define T9_INPUT_Y        0
#define T9_INPUT_H       10
#define T9_GRID_Y        11
#define T9_PREVIEW_IDX   11  // bottom-right cell — preview / submit

// Key 9 (*) is the symbol toggle — no char cycling, opens SYM mode
#define T9_KEY_STAR       9
#define T9_KEY_ZERO      10
#define T9_KEY_PREVIEW   11

#define T9_AUTO_MS     1500  // ms before auto-commit

enum T9State {
  T9_IDLE,    // navigating
  T9_CYCLING, // cycling chars on a key, waiting to commit
};

class T9Keyboard {
private:
  Display&  display;
  T9State   state;

  String   inputBuffer;
  uint8_t  cursorKey;      // 0..11
  uint8_t  cycleIndex;     // index into keyChars[cursorKey]
  uint32_t cycleTimer;     // millis() when last SEL was pressed

  // Char maps: [key][cycleIndex]
  // cycleIndex 0 = primary (number/symbol shown on key)
  // cycleIndex 1..n = letters/symbols
  const char* keyChars[T9_NUM_KEYS][6] = {
    {"1", ".", ",", "-", "_", ""},   // 0
    {"2", "a", "b", "c", "",  ""},   // 1
    {"3", "d", "e", "f", "",  ""},   // 2
    {"4", "g", "h", "i", "",  ""},   // 3
    {"5", "j", "k", "l", "",  ""},   // 4
    {"6", "m", "n", "o", "",  ""},   // 5
    {"7", "p", "q", "r", "s", ""},   // 6
    {"8", "t", "u", "v", "",  ""},   // 7
    {"9", "w", "x", "y", "z", ""},   // 8
    {"*", "",  "",  "",  "",  ""},   // 9  (* = SYM, no cycling)
    {"0", " ", "",  "",  "",  ""},   // 10 (0 / space)
    {"",  "",  "",  "",  "",  ""},   // 11 (preview / submit cell)
  };

  uint8_t keyCharCount[T9_NUM_KEYS] = {5, 4, 4, 4, 4, 4, 5, 4, 5, 1, 2, 0};

  // Range hints shown on each key (replaces listing every char)
  const char* keyHint[T9_NUM_KEYS] = {
    ".,_",  // 1
    "a-c",  // 2
    "d-f",  // 3
    "g-i",  // 4
    "j-l",  // 5
    "m-o",  // 6
    "p-s",  // 7
    "t-v",  // 8
    "w-z",  // 9
    "sym",  // *
    "sp",   // 0
    "",     // preview cell
  };

public:
  T9Keyboard(Display& dispRef);

  // Call every loop iteration — handles auto-commit timer
  // Returns true if keyboard is done (submit)
  bool tick(uint32_t now);

  // Call on button events
  // btn: 0=UP 1=SEL 2=DOWN 3=BACK, pressed=just pressed, longPress=held
  // Returns true if keyboard is done (submit or cancel)
  bool onButton(uint8_t btn, bool pressed, bool longPress);

  void draw();
  void reset();

  String getInput() { return inputBuffer; }

private:
  void commitPending();
  void backspace();
  void drawKey(uint8_t keyIdx);
  void drawPreviewCell();
  char currentChar();
};

#endif // HAS_OLED
#endif // T9Keyboard_h

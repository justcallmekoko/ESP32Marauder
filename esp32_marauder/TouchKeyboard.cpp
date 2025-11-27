#include "TouchKeyboard.h"
#include <string.h>
#include <Arduino.h>

#ifdef HAS_TOUCH

extern Display display_obj;

// Keyboard will occupy the bottom half of the screen.
static inline int16_t kbHeight()  { return TFT_HEIGHT / 2; }
static inline int16_t kbYStart()  { return TFT_HEIGHT - kbHeight(); }
static inline int16_t kbWidth()   { return TFT_WIDTH; }
static inline int16_t kbXStart()  { return 0; }

static const int KEY_ROWS = 5;

static const char ROW0_ALPHA[] = "1234567890";
static const char ROW1_ALPHA[] = "qwertyuiop";
static const char ROW2_ALPHA[] = "asdfghjkl";
static const char ROW3_ALPHA[] = "zxcvbnm.";

static const char ROW0_SYM[] = "!@#$%^&*()";
static const char ROW1_SYM[] = "`~-_=+[]{}";
static const char ROW2_SYM[] = "\\|;:'\"";
static const char ROW3_SYM[] = ",.<>/?";

static const uint16_t TOUCH_THRESHOLD = 600;

enum KeyboardLayout {
  LAYOUT_ALPHA = 0,
  LAYOUT_SYMBOLS
};

static void drawTextArea(const char *title, const char *buffer) {
  int16_t areaHeight = TFT_HEIGHT - kbHeight();

  // Clear text area
  display_obj.tft.fillRect(0, 0, TFT_WIDTH, areaHeight, TFT_BLACK);

  int16_t cursorY = 2;

  // Optional title
  if (title && title[0] != '\0') {
    display_obj.tft.setCursor(2, cursorY);
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.print(title);
    cursorY += 16;
  }

  // Draw current text
  display_obj.tft.setCursor(2, cursorY);
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  display_obj.tft.print(buffer ? buffer : "");
}

static void drawKeyboard(KeyboardLayout layout, bool caps) {
  const int16_t kY      = kbYStart();
  const int16_t kH      = kbHeight();
  const int16_t kX      = kbXStart();
  const int16_t kW      = kbWidth();

  const int maxCols = 10;
  const int rows    = KEY_ROWS;

  const int16_t cellW = kW / maxCols;
  const int16_t cellH = kH / rows;

  display_obj.tft.fillRect(kX, kY, kW, kH, TFT_DARKGREY);
  display_obj.tft.setTextColor(TFT_BLACK, TFT_DARKGREY);

  // Choose row strings based on layout
  const char *rowStringsAlpha[4] = {
    ROW0_ALPHA, ROW1_ALPHA, ROW2_ALPHA, ROW3_ALPHA
  };
  const char *rowStringsSym[4] = {
    ROW0_SYM, ROW1_SYM, ROW2_SYM, ROW3_SYM
  };

  const char **rowsPtr = (layout == LAYOUT_ALPHA)
                         ? (const char **)rowStringsAlpha
                         : (const char **)rowStringsSym;

  // Draw normal character rows (0–3)
  for (int r = 0; r < 4; ++r) {
    const char *row = rowsPtr[r];

    // For alpha layout row 3, we will also draw CAPS on the rightmost 2 columns.
    // The textual row only covers the first part.
    int rowLen = strlen(row);

    int16_t rowY = kY + r * cellH;

    // For alpha row 3, we want row chars towards the left, leaving space for CAPS.
    int16_t xOffset;
    if (layout == LAYOUT_ALPHA && r == 3) {
      // Use columns 0..7 for characters, 8..9 for CAPS
      xOffset = 0;
    } else {
      int maxColsRow = maxCols;
      xOffset = (maxColsRow - rowLen);
      if (xOffset < 0) xOffset = 0;
      xOffset = (xOffset * cellW) / 2;  // center row
    }

    for (int i = 0; i < rowLen; ++i) {
      int16_t keyX = kX + xOffset + i * cellW;
      int16_t keyY = rowY;

      // Key border
      display_obj.tft.drawRect(keyX, keyY, cellW, cellH, TFT_BLACK);

      // Label
      display_obj.tft.setCursor(keyX + cellW / 2 - 3, keyY + cellH / 2 - 4);
      char c = row[i];

      // Apply CAPS on alpha letters
      if (layout == LAYOUT_ALPHA && r > 0) {
        if (c >= 'a' && c <= 'z') {
          if (caps) {
            c = (char)(c - 'a' + 'A');
          }
        }
      }

      char s[2] = { c, '\0' };
      display_obj.tft.print(s);
    }

    // Draw CAPS key for alpha layout on row 3 (rightmost two columns)
    if (layout == LAYOUT_ALPHA && r == 3) {
      int16_t capsX = kX + 8 * cellW;
      int16_t capsW = 2 * cellW;

      display_obj.tft.drawRect(capsX, rowY, capsW, cellH, TFT_BLACK);
      display_obj.tft.setCursor(capsX + 4, rowY + cellH / 2 - 4);
      // Show different label based on state
      if (caps) {
        display_obj.tft.print("caps");
      } else {
        display_obj.tft.print("CAPS");
      }
    }
  }

  // Special row (row index 4): CANCEL | SYMB/ABC | SPACE | BKSP | OK
  int r = 4;
  int16_t rowY = kY + r * cellH;

  // Divide width into 6 segments:
  // [CANCEL][SYMB][SPACE (2 segments)][BKSP][OK]
  int16_t segW  = kW / 6;
  int16_t x0 = kX;
  int16_t x1 = x0 + segW;         // end CANCEL
  int16_t x2 = x1 + segW;         // end SYMB
  int16_t x3 = x2 + 2 * segW;     // end SPACE
  int16_t x4 = x3 + segW;         // end BKSP
  int16_t x5 = x4 + segW;         // end OK

  // CANCEL
  display_obj.tft.drawRect(x0, rowY, segW, cellH, TFT_BLACK);
  display_obj.tft.setCursor(x0 + 4, rowY + cellH / 2 - 4);
  display_obj.tft.print("CANCEL");

  // SYMB / ABC
  display_obj.tft.drawRect(x1, rowY, segW, cellH, TFT_BLACK);
  display_obj.tft.setCursor(x1 + 4, rowY + cellH / 2 - 4);
  if (layout == LAYOUT_ALPHA) {
    display_obj.tft.print("SYMB");
  } else {
    display_obj.tft.print("ABC");
  }

  display_obj.tft.drawRect(x2, rowY, (x3 - x2), cellH, TFT_BLACK);
  display_obj.tft.setCursor(x2 + 4, rowY + cellH / 2 - 4);
  display_obj.tft.print("SPACE");

  display_obj.tft.drawRect(x3, rowY, (x4 - x3), cellH, TFT_BLACK);
  display_obj.tft.setCursor(x3 + 4, rowY + cellH / 2 - 4);
  display_obj.tft.print("BKSP");

  display_obj.tft.drawRect(x4, rowY, (x5 - x4), cellH, TFT_BLACK);
  display_obj.tft.setCursor(x4 + 4, rowY + cellH / 2 - 4);
  display_obj.tft.print("OK");
}

static bool appendChar(char *buffer, size_t bufLen, char c) {
  size_t len = strlen(buffer);
  if (len + 1 < bufLen) {
    buffer[len] = c;
    buffer[len + 1] = '\0';
    return true;
  }
  return false;
}

static KeyboardResult handleKeyboardTouch(uint16_t tx, uint16_t ty,
                                          char *buffer, size_t bufLen,
                                          KeyboardLayout layout,
                                          bool caps) {
  if (!buffer || bufLen < 2) return KB_NONE;

  const int16_t kY      = kbYStart();
  const int16_t kH      = kbHeight();
  const int16_t kX      = kbXStart();
  const int16_t kW      = kbWidth();

  if (ty < kY || ty >= (kY + kH)) {
    // Touch is outside keyboard area
    return KB_NONE;
  }

  const int maxCols = 10;
  const int rows    = KEY_ROWS;
  const int16_t cellW = kW / maxCols;
  const int16_t cellH = kH / rows;

  int rowIndex = (ty - kY) / cellH;

  // Choose row strings based on layout
  const char *rowStringsAlpha[4] = {
    ROW0_ALPHA, ROW1_ALPHA, ROW2_ALPHA, ROW3_ALPHA
  };
  const char *rowStringsSym[4] = {
    ROW0_SYM, ROW1_SYM, ROW2_SYM, ROW3_SYM
  };

  const char **rowsPtr = (layout == LAYOUT_ALPHA)
                         ? (const char **)rowStringsAlpha
                         : (const char **)rowStringsSym;

  // Normal rows (0..3)
  if (rowIndex >= 0 && rowIndex <= 3) {
    const char *row = rowsPtr[rowIndex];
    int rowLen = strlen(row);

    int16_t rowY = kY + rowIndex * cellH;

    // Alpha row 3: characters on columns 0..7, CAPS on columns 8..9
    if (layout == LAYOUT_ALPHA && rowIndex == 3) {
      int16_t capsXStart = kX + 8 * cellW;
      int16_t capsXEnd   = capsXStart + 2 * cellW;

      // Check CAPS region
      if (tx >= capsXStart && tx < capsXEnd) {
        return KB_TOGGLE_CAPS;
      }

      // Characters only in the columns before CAPS
      int16_t xOffset = 0;
      if (tx < kX || tx >= (kX + 8 * cellW)) {
        return KB_NONE;
      }

      int colIndex = (tx - (kX + xOffset)) / cellW;
      if (colIndex < 0 || colIndex >= rowLen) return KB_NONE;

      char c = row[colIndex];
      // Apply caps mapping
      if (c >= 'a' && c <= 'z' && caps) {
        c = (char)(c - 'a' + 'A');
      }

      if (appendChar(buffer, bufLen, c)) {
        return KB_CHANGED;
      }
      return KB_NONE;
    }

    // All other rows
    int16_t xOffset = (maxCols - rowLen);
    if (xOffset < 0) xOffset = 0;
    xOffset = (xOffset * cellW) / 2;

    if (tx < kX + xOffset || tx >= kX + xOffset + rowLen * cellW) {
      return KB_NONE;
    }

    int colIndex = (tx - (kX + xOffset)) / cellW;
    if (colIndex < 0 || colIndex >= rowLen) return KB_NONE;

    char c = row[colIndex];

    // Apply caps on alpha letters (rows 1 and 2)
    if (layout == LAYOUT_ALPHA && rowIndex > 0) {
      if (c >= 'a' && c <= 'z' && caps) {
        c = (char)(c - 'a' + 'A');
      }
    }

    if (appendChar(buffer, bufLen, c)) {
      return KB_CHANGED;
    }
    return KB_NONE;
  }

  // Special row (rowIndex == 4): CANCEL | SYMB/ABC | SPACE | BKSP | OK
  if (rowIndex == 4) {
    int16_t rowY = kY + rowIndex * cellH;

    int16_t segW  = kW / 6;
    int16_t x0 = kX;
    int16_t x1 = x0 + segW;         // end CANCEL
    int16_t x2 = x1 + segW;         // end SYMB
    int16_t x3 = x2 + 2 * segW;     // end SPACE
    int16_t x4 = x3 + segW;         // end BKSP
    int16_t x5 = x4 + segW;         // end OK

    // CANCEL
    if (tx >= x0 && tx < x1) {
      return KB_CANCEL;
    }

    // SYMB / ABC toggle
    if (tx >= x1 && tx < x2) {
      return KB_TOGGLE_LAYOUT;
    }

    // SPACE (x2..x3)
    if (tx >= x2 && tx < x3) {
      if (appendChar(buffer, bufLen, ' ')) {
        return KB_CHANGED;
      }
      return KB_NONE;
    }

    // BKSP (x3..x4)
    if (tx >= x3 && tx < x4) {
      size_t len = strlen(buffer);
      if (len > 0) {
        buffer[len - 1] = '\0';
        return KB_CHANGED;
      }
      return KB_NONE;
    }

    // OK (x4..x5)
    if (tx >= x4 && tx < x5) {
      return KB_DONE;
    }
  }

  return KB_NONE;
}


bool keyboardInput(char *buffer, size_t bufLen, const char *title) {
  if (!buffer || bufLen < 2) {
    return false;
  }

  // To force clear, uncomment this:
  // buffer[0] = '\0';

  KeyboardLayout layout = LAYOUT_ALPHA;
  bool caps = false;

  drawTextArea(title, buffer);
  drawKeyboard(layout, caps);

  uint32_t lastTouchTime = 0;
  const uint32_t debounceMs = 120;

  while (true) {
    uint16_t x = 0, y = 0;
    uint8_t touched = display_obj.updateTouch(&x, &y, TOUCH_THRESHOLD);

    if (touched) {
      uint32_t now = millis();
      if (now - lastTouchTime < debounceMs) {
        // "debounce"
        continue;
      }
      lastTouchTime = now;

      KeyboardResult r = handleKeyboardTouch(x, y, buffer, bufLen, layout, caps);

      if (r == KB_CHANGED) {
        drawTextArea(title, buffer);
      } else if (r == KB_DONE) {
        drawTextArea(title, buffer);
        return true;
      } else if (r == KB_CANCEL) {
        // Optional: buffer[0] = '\0';
        drawTextArea(title, buffer);
        return false;
      } else if (r == KB_TOGGLE_LAYOUT) {
        layout = (layout == LAYOUT_ALPHA) ? LAYOUT_SYMBOLS : LAYOUT_ALPHA;
        drawKeyboard(layout, caps);
      } else if (r == KB_TOGGLE_CAPS) {
        caps = !caps;
        drawKeyboard(layout, caps);
      }
    }

    delay(5);
    yield();
  }

  return false;
}

#endif
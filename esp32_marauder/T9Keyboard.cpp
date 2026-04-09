#include "T9Keyboard.h"

#ifdef HAS_OLED

T9Keyboard::T9Keyboard(Display& dispRef)
  : display(dispRef), state(T9_IDLE),
    inputBuffer(""), cursorKey(0), cycleIndex(0), cycleTimer(0) {
  Serial.printf("[T9] constructed state=%d cycleIndex=%d\n", state, cycleIndex);
}

void T9Keyboard::reset() {
  state       = T9_IDLE;
  inputBuffer = "";
  cursorKey   = 0;
  cycleIndex  = 0;
  cycleTimer  = 0;
}

char T9Keyboard::currentChar() {
  if (keyCharCount[cursorKey] == 0) return 0;
  return keyChars[cursorKey][cycleIndex][0];
}

void T9Keyboard::commitPending() {
  if (state != T9_CYCLING) return;
  char ch = currentChar();
  if (ch && (ch != ' ' || inputBuffer.length() > 0))
    inputBuffer += ch;
  cycleIndex = 0;
  state      = T9_IDLE;
}

void T9Keyboard::backspace() {
  if (state == T9_CYCLING) {
    // Cancel pending char instead of deleting
    cycleIndex = 0;
    state      = T9_IDLE;
  } else if (inputBuffer.length() > 0) {
    inputBuffer.remove(inputBuffer.length() - 1);
  }
}

// ---- tick — call every loop iteration for auto-commit timer ----

bool T9Keyboard::tick(uint32_t now) {
  if (state == T9_CYCLING && (now - cycleTimer) >= T9_AUTO_MS) {
    commitPending();
    draw();
  }
  return false;
}

// ---- button handler ----

bool T9Keyboard::onButton(uint8_t btn, bool pressed, bool longPress) {
  if (!pressed && !longPress) return false;

  if (btn == 0 || btn == 2) {  // UP / DOWN — navigate
    commitPending();
    if (btn == 0)
      cursorKey = (cursorKey == 0) ? T9_NUM_KEYS - 1 : cursorKey - 1;
    else
      cursorKey = (cursorKey + 1) % T9_NUM_KEYS;
    cycleIndex = 0;
    draw();
    return false;
  }

  if (btn == 3) {  // BACK
    if (longPress) return true;  // cancel / exit
    backspace();
    draw();
    return false;
  }


  if (btn == 1) {  // SELECT
    // Preview/submit cell
    if (cursorKey == T9_KEY_PREVIEW) {
      commitPending();
      return true;  // submit
    }

    // Star key — TODO: symbol menu
    if (cursorKey == T9_KEY_STAR) {
      // placeholder: do nothing for now
      return false;
    }

    if (longPress) {
      // Long press = immediately commit current cycling char
      commitPending();
      draw();
      return false;
    }

    // Short press = cycle to next char, restart timer
    if (state == T9_CYCLING && cursorKey == cursorKey) {
      // Already cycling this key
      cycleIndex = (cycleIndex + 1) % keyCharCount[cursorKey];
    } else {
      // Start cycling — skip index 0 (the number) to go straight to letters
      cycleIndex = 1 % keyCharCount[cursorKey];
      state = T9_CYCLING;
    }
    cycleTimer = millis();
    draw();
    return false;
  }

  return false;
}

// ---- rendering ----

void T9Keyboard::drawKey(uint8_t keyIdx) {
  uint8_t  row = keyIdx / T9_KEYS_PER_ROW;
  uint8_t  col = keyIdx % T9_KEYS_PER_ROW;
  uint16_t x   = col * T9_KEY_WIDTH + 1;
  uint16_t y   = T9_GRID_Y + row * T9_KEY_HEIGHT;
  uint16_t w   = T9_KEY_WIDTH - 1;
  uint16_t h   = T9_KEY_HEIGHT - 1;

  bool highlight = (keyIdx == cursorKey);

  if (highlight) {
    display.oled.fillRect(x, y, w, h, SH110X_WHITE);
    display.oled.setTextColor(SH110X_BLACK);
  } else {
    display.oled.drawRect(x, y, w, h, SH110X_WHITE);
    display.oled.setTextColor(SH110X_WHITE);
  }

  display.oled.setTextSize(1);

  // Primary label (number)
  display.oled.setCursor(x + 2, y + 3);
  if (keyIdx == T9_KEY_STAR)
    display.oled.print("*");
  else if (keyIdx == T9_KEY_ZERO)
    display.oled.print("0");
  else if (keyIdx < 9)
    display.oled.print(keyIdx + 1);

  // Hint (letter range or "sym" / "sp")
  display.oled.setCursor(x + 14, y + 3);
  display.oled.print(keyHint[keyIdx]);
}

void T9Keyboard::drawPreviewCell() {
  uint8_t  col = T9_KEY_PREVIEW % T9_KEYS_PER_ROW;
  uint8_t  row = T9_KEY_PREVIEW / T9_KEYS_PER_ROW;
  uint16_t x   = col * T9_KEY_WIDTH + 1;
  uint16_t y   = T9_GRID_Y + row * T9_KEY_HEIGHT;
  uint16_t w   = T9_KEY_WIDTH - 1;
  uint16_t h   = T9_KEY_HEIGHT - 1;

  bool highlight = (cursorKey == T9_KEY_PREVIEW);

  display.oled.fillRect(x, y, w, h, highlight ? SH110X_WHITE : SH110X_BLACK);
  display.oled.drawRect(x, y, w, h, SH110X_WHITE);
  display.oled.setTextColor(highlight ? SH110X_BLACK : SH110X_WHITE);
  display.oled.setTextSize(1);

  if (state == T9_CYCLING) {
    // Show the char currently under the cursor — large and centred
    char ch = currentChar();
    display.oled.setTextSize(1);
    display.oled.setCursor(x + 8, y + 3);
    display.oled.print(ch ? ch : ' ');
    // Show cycle position dots: e.g. "-o-" for middle of 3
    // Keep it simple: show cycleIndex/total as tiny indicator
    display.oled.setCursor(x + 20, y + 3);
    display.oled.print(cycleIndex);
    display.oled.print("/");
    display.oled.print(keyCharCount[cursorKey] - 1);
  } else {
    // Idle: show "OK" centred
    display.oled.setCursor(x + 12, y + 3);
    display.oled.print("OK");
  }
}

void T9Keyboard::draw() {
  display.oled.clearDisplay();

  // Input bar — inverted strip at top
  display.oled.fillRect(0, T9_INPUT_Y, 128, T9_INPUT_H, SH110X_WHITE);
  display.oled.setTextColor(SH110X_BLACK);
  display.oled.setTextSize(1);
  display.oled.setCursor(2, T9_INPUT_Y + 1);

  // Show input + pending char (if cycling)
  String shown = inputBuffer;
  if (state == T9_CYCLING) {
    char ch = currentChar();
    if (ch) shown += ch;
  }
  shown += "_";
  // Scroll left if too long (21 chars visible at font size 1)
  if (shown.length() > 21) shown = shown.substring(shown.length() - 21);
  display.oled.print(shown);

  // Draw 11 normal keys
  for (uint8_t i = 0; i < T9_KEY_PREVIEW; i++) {
    drawKey(i);
  }

  // Draw preview/submit cell
  drawPreviewCell();

  display.oled.display();
}

#endif // HAS_OLED

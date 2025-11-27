#include "TouchKeyboard.h"
#include <string.h>   // for strlen, memset

#ifdef HAS_TOUCH

// display_obj is created in your main .ino
extern Display display_obj;

// ---- Layout config ----

// Keyboard will occupy the bottom half of the screen.
static inline int16_t kbHeight()  { return TFT_HEIGHT / 2; }
static inline int16_t kbYStart()  { return TFT_HEIGHT - kbHeight(); }
static inline int16_t kbWidth()   { return TFT_WIDTH; }
static inline int16_t kbXStart()  { return 0; }

// We’ll do 4 rows of “normal” keys and 1 special row = 5 rows total
static const int KEY_ROWS = 5;

// Character rows (top to bottom of the keyboard area)
static const char ROW0[] = "1234567890";
static const char ROW1[] = "QWERTYUIOP";
static const char ROW2[] = "ASDFGHJKL";
static const char ROW3[] = "ZXCVBNM.-_";

// Special row: [SPACE] [BKSP] [OK] – handled separately

// Touch threshold (adjust if needed)
static const uint16_t TOUCH_THRESHOLD = 600;

// --- Helper: draw text area (top half) ---

static void drawTextArea(const char *title, const char *buffer)
{
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

// --- Helper: draw entire keyboard ---

static void drawKeyboard()
{
    const int16_t kY      = kbYStart();
    const int16_t kH      = kbHeight();
    const int16_t kX      = kbXStart();
    const int16_t kW      = kbWidth();

    const int maxCols = 10;        // base grid is 10 columns
    const int rows    = KEY_ROWS;  // 4 char rows + 1 special row

    const int16_t cellW = kW / maxCols;
    const int16_t cellH = kH / rows;

    display_obj.tft.fillRect(kX, kY, kW, kH, TFT_DARKGREY);

    display_obj.tft.setTextColor(TFT_BLACK, TFT_DARKGREY);

    // Draw normal character rows (0–3)
    const char *rowStrings[4] = { ROW0, ROW1, ROW2, ROW3 };

    for (int r = 0; r < 4; ++r) {
        const char *row = rowStrings[r];
        int rowLen = strlen(row);

        int16_t rowY = kY + r * cellH;
        int16_t xOffset = (maxCols - rowLen);
        if (xOffset < 0) xOffset = 0;
        xOffset = (xOffset * cellW) / 2;  // center row

        for (int i = 0; i < rowLen; ++i) {
            int16_t keyX = kX + xOffset + i * cellW;
            int16_t keyY = rowY;

            // Key border
            display_obj.tft.drawRect(keyX, keyY, cellW, cellH, TFT_BLACK);

            // Label
            display_obj.tft.setCursor(keyX + cellW / 2 - 3, keyY + cellH / 2 - 4);
            char c[2] = { row[i], '\0' };
            display_obj.tft.print(c);
        }
    }

    // Special row (row index 4): SPACE | BKSP | OK
    int r = 4;
    int16_t rowY = kY + r * cellH;

    // We’ll divide width into 3 segments
    int16_t spaceW = kW / 2;           // left half for SPACE
    int16_t bkspW  = kW / 4;           // next quarter for BKSP
    int16_t okW    = kW - spaceW - bkspW; // remaining for OK

    int16_t keyX = kX;
    // SPACE
    display_obj.tft.drawRect(keyX, rowY, spaceW, cellH, TFT_BLACK);
    display_obj.tft.setCursor(keyX + 4, rowY + cellH / 2 - 4);
    display_obj.tft.print("SPACE");

    keyX += spaceW;
    // BKSP
    display_obj.tft.drawRect(keyX, rowY, bkspW, cellH, TFT_BLACK);
    display_obj.tft.setCursor(keyX + 4, rowY + cellH / 2 - 4);
    display_obj.tft.print("BKSP");

    keyX += bkspW;
    // OK
    display_obj.tft.drawRect(keyX, rowY, okW, cellH, TFT_BLACK);
    display_obj.tft.setCursor(keyX + okW / 2 - 6, rowY + cellH / 2 - 4);
    display_obj.tft.print("OK");
}

// --- Helper: process a touch and update buffer ---

static KeyboardResult handleKeyboardTouch(uint16_t tx, uint16_t ty,
                                          char *buffer, size_t bufLen)
{
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

    // Normal rows (0..3)
    if (rowIndex >= 0 && rowIndex <= 3) {
        const char *rowStrings[4] = { ROW0, ROW1, ROW2, ROW3 };
        const char *row = rowStrings[rowIndex];
        int rowLen = strlen(row);

        int16_t rowY = kY + rowIndex * cellH;

        int16_t xOffset = (maxCols - rowLen);
        if (xOffset < 0) xOffset = 0;
        xOffset = (xOffset * cellW) / 2;

        // Determine which key in this row
        if (tx < kX + xOffset || tx >= kX + xOffset + rowLen * cellW) {
            return KB_NONE;
        }

        int colIndex = (tx - (kX + xOffset)) / cellW;
        if (colIndex < 0 || colIndex >= rowLen) return KB_NONE;

        char c = row[colIndex];

        // Append char if space
        size_t len = strlen(buffer);
        if (len + 1 < bufLen) {
            buffer[len] = c;
            buffer[len + 1] = '\0';
            return KB_CHANGED;
        }
        return KB_NONE;
    }

    // Special row (rowIndex == 4): SPACE | BKSP | OK
    if (rowIndex == 4) {
        int16_t rowY = kY + rowIndex * cellH;

        int16_t spaceW = kW / 2;
        int16_t bkspW  = kW / 4;
        int16_t okW    = kW - spaceW - bkspW;

        int16_t x0 = kX;
        int16_t x1 = x0 + spaceW;
        int16_t x2 = x1 + bkspW;
        int16_t x3 = x2 + okW;

        // SPACE
        if (tx >= x0 && tx < x1) {
            size_t len = strlen(buffer);
            if (len + 1 < bufLen) {
                buffer[len] = ' ';
                buffer[len + 1] = '\0';
                return KB_CHANGED;
            }
            return KB_NONE;
        }

        // BKSP
        if (tx >= x1 && tx < x2) {
            size_t len = strlen(buffer);
            if (len > 0) {
                buffer[len - 1] = '\0';
                return KB_CHANGED;
            }
            return KB_NONE;
        }

        // OK
        if (tx >= x2 && tx < x3) {
            return KB_DONE;
        }
    }

    return KB_NONE;
}

// --- Public API: blocking keyboard input ---

bool keyboardInput(char *buffer, size_t bufLen, const char *title)
{
    if (!buffer || bufLen < 2) {
        return false;
    }

    // Preserve existing buffer content (caller may prefill) or clear:
    // buffer[0] = '\0';

    drawTextArea(title, buffer);
    drawKeyboard();

    uint32_t lastTouchTime = 0;
    const uint32_t debounceMs = 120;

    while (true) {
        uint16_t x = 0, y = 0;
        uint8_t touched = display_obj.updateTouch(&x, &y, TOUCH_THRESHOLD);

        if (touched) {
            uint32_t now = millis();
            if (now - lastTouchTime < debounceMs) {
                // crude debounce
                continue;
            }
            lastTouchTime = now;

            KeyboardResult r = handleKeyboardTouch(x, y, buffer, bufLen);

            if (r == KB_CHANGED) {
                drawTextArea(title, buffer);
            } else if (r == KB_DONE) {
                // Redraw once more to ensure final text shown (optional)
                drawTextArea(title, buffer);
                return true;
            }
        }

        // Allow other tasks / WiFi / BLE to run
        delay(5);
        yield();
    }

    // Unreachable
    // return false;
}

#endif
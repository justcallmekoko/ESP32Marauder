#include "Keyboard.h"

#if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)

#ifdef MARAUDER_CARDPUTER
#include <driver/gpio.h>
#include <Arduino.h>

#define digitalWrite(pin, level) gpio_set_level((gpio_num_t)pin, level)
#define digitalRead(pin) gpio_get_level((gpio_num_t)pin)

void Keyboard_Class::_set_output(const std::vector<int> &pinList,
                                 uint8_t output)
{
    output = output & 0B00000111;

    digitalWrite(pinList[0], (output & 0B00000001));
    digitalWrite(pinList[1], (output & 0B00000010));
    digitalWrite(pinList[2], (output & 0B00000100));
}

uint8_t Keyboard_Class::_get_input(const std::vector<int> &pinList)
{
    uint8_t buffer = 0x00;
    uint8_t pin_value = 0x00;

    for (int i = 0; i < 7; i++)
    {
        pin_value = (digitalRead(pinList[i]) == 1) ? 0x00 : 0x01;
        pin_value = pin_value << i;
        buffer = buffer | pin_value;
    }

    return buffer;
}
#endif

#ifdef MARAUDER_CARDPUTER_ADV
volatile bool Keyboard_Class::_tca_interrupt = false;

void IRAM_ATTR Keyboard_Class::_tca_isr() {
    _tca_interrupt = true;
}
#endif

void Keyboard_Class::begin()
{
#ifdef MARAUDER_CARDPUTER
    for (auto i : output_list)
    {
        gpio_reset_pin((gpio_num_t)i);
        gpio_set_direction((gpio_num_t)i, GPIO_MODE_OUTPUT);
        gpio_set_pull_mode((gpio_num_t)i, GPIO_PULLUP_PULLDOWN);
        digitalWrite(i, 0);
    }

    for (auto i : input_list)
    {
        gpio_reset_pin((gpio_num_t)i);
        gpio_set_direction((gpio_num_t)i, GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)i, GPIO_PULLUP_ONLY);
    }

    _set_output(output_list, 0);
#elif defined(MARAUDER_CARDPUTER_ADV)
    Wire.begin(8, 9);  // SDA=GPIO8, SCL=GPIO9
    _tca_initialized = _tca8418.begin(TCA8418_DEFAULT_ADDR, &Wire);
    if (_tca_initialized) {
        _tca8418.matrix(7, 8);  // 7 rows x 8 cols
        _tca8418.flush();
        _tca8418.enableInterrupts();
        pinMode(11, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(11), _tca_isr, FALLING);
        _tca_interrupt = true;  // Force initial scan
    } else {
        Serial.println("[ERROR] TCA8418 keyboard not found on I2C (addr 0x34)");
    }
#endif
}

uint8_t Keyboard_Class::getKey(Point2D_t keyCoor)
{
    uint8_t ret = 0;

    if ((keyCoor.x < 0) || (keyCoor.y < 0))
    {
        return 0;
    }
    if (_keys_state_buffer.ctrl || _keys_state_buffer.shift ||
        _is_caps_locked)
    {
        ret = _key_value_map[keyCoor.y][keyCoor.x].value_second;
    }
    else
    {
        ret = _key_value_map[keyCoor.y][keyCoor.x].value_first;
    }
    return ret;
}

void Keyboard_Class::updateKeyList()
{
    _key_list_buffer.clear();
    Point2D_t coor;

#ifdef MARAUDER_CARDPUTER
    uint8_t input_value = 0;

    for (int i = 0; i < 8; i++)
    {
        _set_output(output_list, i);
        input_value = _get_input(input_list);
        /* If key pressed */

        if (input_value)
        {
            /* Get X */
            for (int j = 0; j < 7; j++)
            {
                if (input_value & (0x01 << j))
                {
                    coor.x = (i > 3) ? X_map_chart[j].x_1 : X_map_chart[j].x_2;

                    /* Get Y */
                    coor.y = (i > 3) ? (i - 4) : i;
                    // printf("%d,%d\t", coor.x, coor.y);

                    /* Keep the same as picture */
                    coor.y = -coor.y;
                    coor.y = coor.y + 3;

                    _key_list_buffer.push_back(coor);
                }
            }
        }
    }
#elif defined(MARAUDER_CARDPUTER_ADV)
    if (!_tca_initialized) return;

    // Drain TCA8418 FIFO when interrupt signals new events
    if (_tca_interrupt) {
        _tca_interrupt = false;

        int evt;
        while ((evt = _tca8418.getEvent()) != 0) {
            // Bit 7 = 1 means key press per TCA8418 datasheet (SCPS162).
            // Note: the vendored Adafruit library comments have this backwards.
            bool pressed = (evt & 0x80) != 0;
            int key = (evt & 0x7F) - 1;  // Convert from 1-indexed key code to 0-indexed
            if (key < 0 || key >= 70) continue;
            // TCA8418 uses 10-column stride internally
            int row = key / 10;
            int col = key % 10;
            if (row >= 7 || col >= 8) continue;

            // Remap to match _key_value_map[4][14] coordinate system
            coor.x = (row * 2) + (col > 3 ? 1 : 0);
            coor.y = col % 4;

            if (coor.x >= 14 || coor.y >= 4) continue;

            if (pressed) {
                // Prevent duplicate tracking (e.g. on missed release event)
                bool already = false;
                for (auto &k : _tca_pressed_keys) {
                    if (k.x == coor.x && k.y == coor.y) { already = true; break; }
                }
                if (!already) _tca_pressed_keys.push_back(coor);
            } else {
                // Remove released key from tracked state
                for (auto it = _tca_pressed_keys.begin(); it != _tca_pressed_keys.end(); ++it) {
                    if (it->x == coor.x && it->y == coor.y) {
                        _tca_pressed_keys.erase(it);
                        break;
                    }
                }
            }
        }
        // Check for FIFO overflow (bit 3) and recover
        uint8_t int_stat = _tca8418.readRegister(TCA8418_REG_INT_STAT);
        if (int_stat & 0x08) {
            _tca8418.flush();
            _tca_pressed_keys.clear();
        }
        // Clear all INT_STAT bits so the INT pin deasserts and can fire again
        _tca8418.writeRegister(TCA8418_REG_INT_STAT, 0x0F);
    }

    // Always populate buffer from currently-held keys
    for (auto &k : _tca_pressed_keys) {
        _key_list_buffer.push_back(k);
    }
#endif
}

uint8_t Keyboard_Class::isPressed()
{
    return _key_list_buffer.size();
}

bool Keyboard_Class::isChange()
{
    if (_last_key_size != _key_list_buffer.size())
    {
        _last_key_size = _key_list_buffer.size();
        return true;
    }
    else
    {
        return false;
    }
}

String Keyboard_Class::getPressedKeysString() {
    updateKeyList();
    String pressed = "";

    for (auto &keyCoor : _key_list_buffer) {
        pressed += getKey(keyCoor);
    }

    return pressed;
}

bool Keyboard_Class::isKeyPressed(char c)
{
    if (_key_list_buffer.size())
    {
        for (const auto &i : _key_list_buffer)
        {
            if (getKey(i) == c)
                return true;
        }
    }
    return false;
}

#include <cstring>

void Keyboard_Class::updateKeysState()
{
    _keys_state_buffer.reset();
    _key_pos_print_keys.clear();
    _key_pos_hid_keys.clear();
    _key_pos_modifier_keys.clear();

    // Get special keys
    for (auto &i : _key_list_buffer)
    {
        // modifier
        if (getKeyValue(i).value_first == KEY_FN)
        {
            _keys_state_buffer.fn = true;
            continue;
        }
        if (getKeyValue(i).value_first == KEY_OPT)
        {
            _keys_state_buffer.opt = true;
            continue;
        }

        if (getKeyValue(i).value_first == KEY_LEFT_CTRL)
        {
            _keys_state_buffer.ctrl = true;
            _key_pos_modifier_keys.push_back(i);
            continue;
        }

        if (getKeyValue(i).value_first == KEY_LEFT_SHIFT)
        {
            _keys_state_buffer.shift = true;
            _key_pos_modifier_keys.push_back(i);
            continue;
        }

        if (getKeyValue(i).value_first == KEY_LEFT_ALT)
        {
            _keys_state_buffer.alt = true;
            _key_pos_modifier_keys.push_back(i);
            continue;
        }

        // function
        if (getKeyValue(i).value_first == KEY_TAB)
        {
            _keys_state_buffer.tab = true;
            _key_pos_hid_keys.push_back(i);
            continue;
        }

        if (getKeyValue(i).value_first == KEY_BACKSPACE)
        {
            _keys_state_buffer.del = true;
            _key_pos_hid_keys.push_back(i);
            continue;
        }

        if (getKeyValue(i).value_first == KEY_ENTER)
        {
            _keys_state_buffer.enter = true;
            _key_pos_hid_keys.push_back(i);
            continue;
        }

        if (getKeyValue(i).value_first == ' ')
        {
            _keys_state_buffer.space = true;
        }
        _key_pos_hid_keys.push_back(i);
        _key_pos_print_keys.push_back(i);
    }

    for (auto &i : _key_pos_modifier_keys)
    {
        uint8_t key = getKeyValue(i).value_first;
        _keys_state_buffer.modifier_keys.push_back(key);
    }

    for (auto &k : _keys_state_buffer.modifier_keys)
    {
        _keys_state_buffer.modifiers |= (1 << (k - 0x80));
    }

    for (auto &i : _key_pos_hid_keys)
    {
        uint8_t k = getKeyValue(i).value_first;
        if (k == KEY_TAB || k == KEY_BACKSPACE || k == KEY_ENTER)
        {
            _keys_state_buffer.hid_keys.push_back(k);
            continue;
        }
        uint8_t key = _kb_asciimap[k];
        if (key)
        {
            _keys_state_buffer.hid_keys.push_back(key);
        }
    }

    // Deal what left
    for (auto &i : _key_pos_print_keys)
    {
        if (_keys_state_buffer.ctrl || _keys_state_buffer.shift ||
            _is_caps_locked)
        {
            _keys_state_buffer.word.push_back(getKeyValue(i).value_second);
        }
        else
        {
            _keys_state_buffer.word.push_back(getKeyValue(i).value_first);
        }
    }
}

#endif
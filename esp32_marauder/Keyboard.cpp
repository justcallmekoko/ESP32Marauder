#include "Keyboard.h"

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

void Keyboard_Class::begin()
{
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
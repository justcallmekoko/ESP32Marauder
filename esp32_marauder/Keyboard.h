#pragma once

#ifndef Keyboard_h
#define Keyboard_h

#include "configs.h"

#ifdef MARAUDER_CARDPUTER

/**
 * @file keyboard.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-09-22
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <iostream>
#include <vector>
#include "Arduino.h"
#include "Keyboard_def.h"
#include "configs.h"

struct Chart_t
{
    uint8_t value;
    uint8_t x_1;
    uint8_t x_2;
};

struct Point2D_t
{
    int x;
    int y;
};

const std::vector<int> output_list = {8, 9, 11};
const std::vector<int> input_list = {13, 15, 3, 4, 5, 6, 7};

const Chart_t X_map_chart[7] = {{1, 0, 1}, {2, 2, 3}, {4, 4, 5}, {8, 6, 7}, {16, 8, 9}, {32, 10, 11}, {64, 12, 13}};

struct KeyValue_t
{
    const char value_first;
    const char value_second;
};

const KeyValue_t _key_value_map[4][14] = {{{'`', '~'},
                                           {'1', '!'},
                                           {'2', '@'},
                                           {'3', '#'},
                                           {'4', '$'},
                                           {'5', '%'},
                                           {'6', '^'},
                                           {'7', '&'},
                                           {'8', '*'},
                                           {'9', '('},
                                           {'0', ')'},
                                           {'-', '_'},
                                           {'=', '+'},
                                           {KEY_BACKSPACE, KEY_BACKSPACE}},
                                          {{KEY_TAB, KEY_TAB},
                                           {'q', 'Q'},
                                           {'w', 'W'},
                                           {'e', 'E'},
                                           {'r', 'R'},
                                           {'t', 'T'},
                                           {'y', 'Y'},
                                           {'u', 'U'},
                                           {'i', 'I'},
                                           {'o', 'O'},
                                           {'p', 'P'},
                                           {'[', '{'},
                                           {']', '}'},
                                           {'\\', '|'}},
                                          {{KEY_FN, KEY_FN},
                                           {KEY_LEFT_SHIFT, KEY_LEFT_SHIFT},
                                           {'a', 'A'},
                                           {'s', 'S'},
                                           {'d', 'D'},
                                           {'f', 'F'},
                                           {'g', 'G'},
                                           {'h', 'H'},
                                           {'j', 'J'},
                                           {'k', 'K'},
                                           {'l', 'L'},
                                           {';', ':'},
                                           {'\'', '\"'},
                                           {KEY_ENTER, KEY_ENTER}},
                                          {{KEY_LEFT_CTRL, KEY_LEFT_CTRL},
                                           {KEY_OPT, KEY_OPT},
                                           {KEY_LEFT_ALT, KEY_LEFT_ALT},
                                           {'z', 'Z'},
                                           {'x', 'X'},
                                           {'c', 'C'},
                                           {'v', 'V'},
                                           {'b', 'B'},
                                           {'n', 'N'},
                                           {'m', 'M'},
                                           {',', '<'},
                                           {'.', '>'},
                                           {'/', '?'},
                                           {' ', ' '}}};

class Keyboard_Class
{
public:
    struct KeysState
    {
        bool tab = false;
        bool fn = false;
        bool shift = false;
        bool ctrl = false;
        bool opt = false;
        bool alt = false;
        bool del = false;
        bool enter = false;
        bool space = false;
        uint8_t modifiers = 0;

        std::vector<char> word;
        std::vector<uint8_t> hid_keys;
        std::vector<uint8_t> modifier_keys;

        void reset()
        {
            tab = false;
            fn = false;
            shift = false;
            ctrl = false;
            opt = false;
            alt = false;
            del = false;
            enter = false;
            space = false;
            modifiers = 0;
            word.clear();
            hid_keys.clear();
            modifier_keys.clear();
        }
    };

private:
    std::vector<Point2D_t> _key_list_buffer;
    std::vector<Point2D_t> _key_pos_print_keys; // only text: eg A,B,C
    std::vector<Point2D_t> _key_pos_hid_keys;   // print key + space, enter, del
    std::vector<Point2D_t>
        _key_pos_modifier_keys; // modifier key: eg shift, ctrl, alt
    KeysState _keys_state_buffer;
    bool _is_caps_locked;
    uint8_t _last_key_size;

    void _set_output(const std::vector<int> &pinList, uint8_t output);
    uint8_t _get_input(const std::vector<int> &pinList);

public:
    const char _ascii_list[95] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p',
                            'q','r','s','t','u','v','w','x','y','z','A','B','C','D','E','F',
                            'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
                            'W','X','Y','Z',' ','0','1','2','3','4','5','6','7','8','9','-',
                            '=','[',']',';','\'',',','.','/','`','\\','_','+','{','}',':',
                            '"','<','>','?','~','|','!','@','#','$','%','^','&','*','(',')'};
                            
    Keyboard_Class() : _is_caps_locked(false)
    {
    }

    void begin();
    uint8_t getKey(Point2D_t keyCoor);

    void updateKeyList();
    inline std::vector<Point2D_t> &keyList()
    {
        return _key_list_buffer;
    }

    inline KeyValue_t getKeyValue(const Point2D_t &keyCoor)
    {
        return _key_value_map[keyCoor.y][keyCoor.x];
    }

    uint8_t isPressed();
    bool isChange();
    bool isKeyPressed(char c);
    String getPressedKeysString();

    void updateKeysState();
    inline KeysState &keysState()
    {
        return _keys_state_buffer;
    }

    inline bool capslocked(void)
    {
        return _is_caps_locked;
    }
    inline void setCapsLocked(bool isLocked)
    {
        _is_caps_locked = isLocked;
    }
};
#endif

#endif
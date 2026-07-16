#pragma once

#ifndef lang_var_h
#define lang_var_h

#include "configs.h"
#include "cn_font.h"
#include "lang_var_cn.h"

extern Display display_obj;

// Helper: Read a pre-encoded PROGMEM string into a String for drawCNString
static inline String cnRead(const uint8_t* ptr) {
  String s;
  uint8_t c;
  while ((c = pgm_read_byte(ptr++)) != 0) {
    s += (char)c;
  }
  return s;
}

// Helper: Draw a pre-encoded PROGMEM string at (x, y)
static inline void cnDraw(const uint8_t* ptr, int x, int y) {
  drawCNString(display_obj.tft, cnRead(ptr), x, y);
}

// Helper: Draw a pre-encoded PROGMEM string centered at (x, y)
static inline void cnDrawCentre(const uint8_t* ptr, int x, int y) {
  drawCNCentreString(display_obj.tft, cnRead(ptr), x, y);
}

//Making tables
static PROGMEM const uint8_t *text_table0[] = {text0_0,text0_1, text0_2, text0_3, text0_4, text0_5, text0_6, text0_7, text0_8};
static PROGMEM const uint8_t *text_table1[] = {text1_0,text1_1,text1_2,text1_3,text1_4,text1_5,text1_6,text1_7,text1_8,text1_9,text1_10,text1_11,text1_12,text1_13,text1_14,text1_15,text1_16,text1_17,text1_18,text1_19,text1_20,text1_21,text1_22,text1_23,text1_24,text1_25,text1_26,text1_27,text1_28,text1_29,text1_30,text1_31,text1_32,text1_33,text1_34,text1_35,text1_36,text1_37,text1_38,text1_39,text1_40,text1_41,text1_42,text1_43,text1_44,text1_45,text1_46,text1_47,text1_48,text1_49,text1_50,text1_51,text1_52,text1_53,text1_54,text1_55,text1_56,text1_57,text1_58,text1_59,text1_60,text1_61,text1_62,text1_63,text1_64, text1_65, text1_66, text1_67};
static PROGMEM const uint8_t *text_table2[] = {text2_0,text2_1,text2_2,text2_3,text2_4,text2_5,text2_6,text2_7,text2_8,text2_9,text2_10,text2_11,text2_12,text2_13,text2_14};
static PROGMEM const uint8_t *text_table3[] = {text3_0,text3_1,text3_2,text3_3,text3_4,text3_5};
static PROGMEM const uint8_t *text_table4[] = {text4_0,text4_1,text4_2,text4_3,text4_4,text4_5,text4_6,text4_7,text1_54,text4_9,text4_10,text4_11,text4_12,text4_13,text4_14,text4_15,text4_16,text4_17,text4_18,text4_19,text4_20,text4_21,text4_22,text4_23,text4_24,text4_25,text4_26,text4_27,text4_28,text4_29,text4_30,text4_31,text4_32,text4_33,text4_34,text4_35,text4_36,text4_37,text4_38,text4_39,text4_40,text4_41,text4_42,text4_43,text4_44,text4_45,text4_46,text4_47,text4_48,text4_49};

#endif
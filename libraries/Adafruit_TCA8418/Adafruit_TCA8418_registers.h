#pragma once
/**
 *  @file Adafruit_TCA8418_registers.h
 *
 * 	I2C Driver for the Adafruit TCA8418 Keypad Matrix / GPIO Expander
 *Breakout
 *
 * 	This is a library for the Adafruit TCA8418 breakout:
 * 	https://www.adafruit.com/products/4918
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *
 *	BSD license (see license.txt)
 */

//  REGISTERS

// #define TCA8418_REG_RESERVED          0x00
#define TCA8418_REG_CFG 0x01             ///< Configuration register
#define TCA8418_REG_INT_STAT 0x02        ///< Interrupt status
#define TCA8418_REG_KEY_LCK_EC 0x03      ///< Key lock and event counter
#define TCA8418_REG_KEY_EVENT_A 0x04     ///< Key event register A
#define TCA8418_REG_KEY_EVENT_B 0x05     ///< Key event register B
#define TCA8418_REG_KEY_EVENT_C 0x06     ///< Key event register C
#define TCA8418_REG_KEY_EVENT_D 0x07     ///< Key event register D
#define TCA8418_REG_KEY_EVENT_E 0x08     ///< Key event register E
#define TCA8418_REG_KEY_EVENT_F 0x09     ///< Key event register F
#define TCA8418_REG_KEY_EVENT_G 0x0A     ///< Key event register G
#define TCA8418_REG_KEY_EVENT_H 0x0B     ///< Key event register H
#define TCA8418_REG_KEY_EVENT_I 0x0C     ///< Key event register I
#define TCA8418_REG_KEY_EVENT_J 0x0D     ///< Key event register J
#define TCA8418_REG_KP_LCK_TIMER 0x0E    ///< Keypad lock1 to lock2 timer
#define TCA8418_REG_UNLOCK_1 0x0F        ///< Unlock register 1
#define TCA8418_REG_UNLOCK_2 0x10        ///< Unlock register 2
#define TCA8418_REG_GPIO_INT_STAT_1 0x11 ///< GPIO interrupt status 1
#define TCA8418_REG_GPIO_INT_STAT_2 0x12 ///< GPIO interrupt status 2
#define TCA8418_REG_GPIO_INT_STAT_3 0x13 ///< GPIO interrupt status 3
#define TCA8418_REG_GPIO_DAT_STAT_1 0x14 ///< GPIO data status 1
#define TCA8418_REG_GPIO_DAT_STAT_2 0x15 ///< GPIO data status 2
#define TCA8418_REG_GPIO_DAT_STAT_3 0x16 ///< GPIO data status 3
#define TCA8418_REG_GPIO_DAT_OUT_1 0x17  ///< GPIO data out 1
#define TCA8418_REG_GPIO_DAT_OUT_2 0x18  ///< GPIO data out 2
#define TCA8418_REG_GPIO_DAT_OUT_3 0x19  ///< GPIO data out 3
#define TCA8418_REG_GPIO_INT_EN_1 0x1A   ///< GPIO interrupt enable 1
#define TCA8418_REG_GPIO_INT_EN_2 0x1B   ///< GPIO interrupt enable 2
#define TCA8418_REG_GPIO_INT_EN_3 0x1C   ///< GPIO interrupt enable 3
#define TCA8418_REG_KP_GPIO_1 0x1D       ///< Keypad/GPIO select 1
#define TCA8418_REG_KP_GPIO_2 0x1E       ///< Keypad/GPIO select 2
#define TCA8418_REG_KP_GPIO_3 0x1F       ///< Keypad/GPIO select 3
#define TCA8418_REG_GPI_EM_1 0x20        ///< GPI event mode 1
#define TCA8418_REG_GPI_EM_2 0x21        ///< GPI event mode 2
#define TCA8418_REG_GPI_EM_3 0x22        ///< GPI event mode 3
#define TCA8418_REG_GPIO_DIR_1 0x23      ///< GPIO data direction 1
#define TCA8418_REG_GPIO_DIR_2 0x24      ///< GPIO data direction 2
#define TCA8418_REG_GPIO_DIR_3 0x25      ///< GPIO data direction 3
#define TCA8418_REG_GPIO_INT_LVL_1 0x26  ///< GPIO edge/level detect 1
#define TCA8418_REG_GPIO_INT_LVL_2 0x27  ///< GPIO edge/level detect 2
#define TCA8418_REG_GPIO_INT_LVL_3 0x28  ///< GPIO edge/level detect 3
#define TCA8418_REG_DEBOUNCE_DIS_1 0x29  ///< Debounce disable 1
#define TCA8418_REG_DEBOUNCE_DIS_2 0x2A  ///< Debounce disable 2
#define TCA8418_REG_DEBOUNCE_DIS_3 0x2B  ///< Debounce disable 3
#define TCA8418_REG_GPIO_PULL_1 0x2C     ///< GPIO pull-up disable 1
#define TCA8418_REG_GPIO_PULL_2 0x2D     ///< GPIO pull-up disable 2
#define TCA8418_REG_GPIO_PULL_3 0x2E     ///< GPIO pull-up disable 3
// #define TCA8418_REG_RESERVED          0x2F

//  FIELDS CONFIG REGISTER  1
#define TCA8418_REG_CFG_AI 0x80           ///< Auto-increment for read/write
#define TCA8418_REG_CFG_GPI_E_CGF 0x40    ///< Event mode config
#define TCA8418_REG_CFG_OVR_FLOW_M 0x20   ///< Overflow mode enable
#define TCA8418_REG_CFG_INT_CFG 0x10      ///< Interrupt config
#define TCA8418_REG_CFG_OVR_FLOW_IEN 0x08 ///< Overflow interrupt enable
#define TCA8418_REG_CFG_K_LCK_IEN 0x04    ///< Keypad lock interrupt enable
#define TCA8418_REG_CFG_GPI_IEN 0x02      ///< GPI interrupt enable
#define TCA8418_REG_CFG_KE_IEN 0x01       ///< Key events interrupt enable

//  FIELDS INT_STAT REGISTER  2
#define TCA8418_REG_STAT_CAD_INT 0x10      ///< Ctrl-alt-del seq status
#define TCA8418_REG_STAT_OVR_FLOW_INT 0x08 ///< Overflow interrupt status
#define TCA8418_REG_STAT_K_LCK_INT 0x04    ///< Key lock interrupt status
#define TCA8418_REG_STAT_GPI_INT 0x02      ///< GPI interrupt status
#define TCA8418_REG_STAT_K_INT 0x01        ///< Key events interrupt status

//  FIELDS  KEY_LCK_EC REGISTER 3
#define TCA8418_REG_LCK_EC_K_LCK_EN 0x40 ///< Key lock enable
#define TCA8418_REG_LCK_EC_LCK_2 0x20    ///< Keypad lock status 2
#define TCA8418_REG_LCK_EC_LCK_1 0x10    ///< Keypad lock status 1
#define TCA8418_REG_LCK_EC_KLEC_3 0x08   ///< Key event count bit 3
#define TCA8418_REG_LCK_EC_KLEC_2 0x04   ///< Key event count bit 2
#define TCA8418_REG_LCK_EC_KLEC_1 0x02   ///< Key event count bit 1
#define TCA8418_REG_LCK_EC_KLEC_0 0x01   ///< Key event count bit 0

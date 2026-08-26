#pragma once
//=================================================================
// 小喵掌机 (XiaoMiao) 板型配置 —— 单一真相源
// for ESP32 Marauder (继承官方 MARAUDER_MINI 分支)
//=================================================================
//
// 设计原理:
//   本文件在 configs.h 所有官方 #ifdef MARAUDER_MINI 块执行完毕之后
//   被 #include 进来 (见 configs.h 末尾), 通过 #undef + #define
//   覆盖引脚与屏幕布局, 使小喵硬件配置生效。
//   因此 configs.h 中任何官方 Mini 配置块都 *不需要* 修改 ——
//   上游 configs.h 更新后, 本地只需保留:
//     1) BOARD TARGETS 区的一行:  #define MARAUDER_XIAOMIAO
//     2) configs.h 末尾的一行:    #include "xiaomiao_config.h"
//     3) 本文件 xiaomiao_config.h
//   即可选择 "MARAUDER_XIAOMIAO" 板型直接编译适配。
//
// 使用方法 (任何 Marauder 版本):
//   1. 将本文件放到固件目录 (与 configs.h 同级)。
//   2. 打开 configs.h, 在 //// BOARD TARGETS 区注释掉其它板型,
//      并添加一行:   #define MARAUDER_XIAOMIAO
//   3. 在 configs.h 的 //// HARDWARE NAMES 块里加:
//        #elif defined(MARAUDER_XIAOMIAO)
//          #define HARDWARE_NAME "XiaoMiao Mini"
//   4. 在 configs.h 最末尾 (#endif 之前) 加:
//        #if defined(MARAUDER_XIAOMIAO)
//          #include "xiaomiao_config.h"
//        #endif
//   5. (可选) 在 //// MEMORY LOWER LIMIT STUFF 里加:
//        #elif defined(MARAUDER_XIAOMIAO)
//          #define MEM_LOWER_LIM 10000
//   6. 确认 TFT_eSPI 库 User_Setup_Select.h 选择了
//      User_Setup_marauder_mini.h (沿用官方 Mini 的 ST7735 配置)。
//
// 硬件信息:
//   MCU      : ESP32
//   屏幕      : 1.8" ST7735 160x128 横屏 (ST7735_GREENTAB3, BGR)
//   TF卡      : MicroSD, 与屏幕共享 VSPI 总线
//   按键      : 6键 (上/下/左/右/A确认/B返回)
//   LED      : NeoPixel (与官方 Mini 一致, GPIO25)
//   串口      : GPS 用 Serial2 (外部模块, 可选)
//
// 引脚定义:
//   屏幕: SCK=18 MOSI=23 CS=5 DC=4 RST=-1(不用) BL=直连电源(软件不控)
//   TF卡: SCK=18 MOSI=23 MISO=19 CS=22
//   按键: 上=IO2 下=IO13 左=IO27 右=IO35 A=IO34 B=IO12
//   注: GPIO34/35 为输入-only 引脚, 无内部上拉, 依赖 PCB 外部上拉电阻
//=================================================================

// ---- 0. 板型继承: 让所有 #ifdef MARAUDER_MINI 代码路径自动生效 ----
#ifndef MARAUDER_MINI
  #define MARAUDER_MINI
#endif

// ---- 0b. IDF 版本标记 ----
// ESP32 Arduino Core 3.x 使用 IDF 5.x, 必须走 HAS_IDF_3 兼容路径
// (WiFiScan/esp32_marauder.ino 等据此走 Core 3.x 分支)
#ifndef HAS_IDF_3
  #define HAS_IDF_3
#endif

// ---- 1. 硬件名称 ----
#ifdef HARDWARE_NAME
  #undef HARDWARE_NAME
#endif
#define HARDWARE_NAME "XiaoMiao Mini"

// ---- 2. 按键引脚覆盖 ----
// 官方 Mini 只有 5 键 (L/C/U/R/D); 小喵为 6 键, 增加 B(返回)。
// B_BTN 用 #if defined(B_BTN) 守卫, 避免官方 Mini (未定义 B_BTN) 误编译。
#ifdef L_BTN
  #undef L_BTN
#endif
#ifdef C_BTN
  #undef C_BTN
#endif
#ifdef U_BTN
  #undef U_BTN
#endif
#ifdef R_BTN
  #undef R_BTN
#endif
#ifdef D_BTN
  #undef D_BTN
#endif
#ifdef B_BTN
  #undef B_BTN
#endif

#define L_BTN 27
#define C_BTN 34   // A键 = 确认/选择
#define U_BTN 2
#define R_BTN 35
#define D_BTN 13
#define B_BTN 12   // B键 = 返回上级菜单

#ifdef HAS_L
  #undef HAS_L
#endif
#ifdef HAS_R
  #undef HAS_R
#endif
#ifdef HAS_U
  #undef HAS_U
#endif
#ifdef HAS_D
  #undef HAS_D
#endif
#ifdef HAS_C
  #undef HAS_C
#endif
#ifdef HAS_B
  #undef HAS_B
#endif
#define HAS_L
#define HAS_R
#define HAS_U
#define HAS_D
#define HAS_C
#define HAS_B   // 启用 B 键对象实例化与轮询

// ---- 3. 按键上拉配置 ----
// 所有按键: PCB 外部上拉, 按下时=LOW。
// Switches.cpp: pinMode(pin, INPUT_PULLUP/INPUT_PULLDOWN) 对 GPIO34/35
// (input-only) 的上下拉请求会被硬件忽略, 退化为纯 INPUT, 由外部上拉决定电平。
// getButtonState(): pullup=true 时 LOW=按下, 故全部用 true。
#ifdef L_PULL
  #undef L_PULL
#endif
#ifdef C_PULL
  #undef C_PULL
#endif
#ifdef U_PULL
  #undef U_PULL
#endif
#ifdef R_PULL
  #undef R_PULL
#endif
#ifdef D_PULL
  #undef D_PULL
#endif
#ifdef B_PULL
  #undef B_PULL
#endif

#define L_PULL true
#define C_PULL true   // GPIO34 input-only, 外部上拉, LOW=按下
#define U_PULL true
#define R_PULL true   // GPIO35 input-only, 外部上拉, LOW=按下
#define D_PULL true
#define B_PULL true

// ---- 4. 屏幕引脚覆盖 ----
#ifdef TFT_MISO
  #undef TFT_MISO
#endif
#ifdef TFT_MOSI
  #undef TFT_MOSI
#endif
#ifdef TFT_SCLK
  #undef TFT_SCLK
#endif
#ifdef TFT_CS
  #undef TFT_CS
#endif
#ifdef TFT_DC
  #undef TFT_DC
#endif
#ifdef TFT_RST
  #undef TFT_RST
#endif
#ifdef TOUCH_CS
  #undef TOUCH_CS
#endif

#define TFT_MISO -1   // ST7735 doesn't read back (SD 卡 MISO 用 GPIO19)
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  -1   // 不使用 MCU 控制复位, ST7735 自带 POR 上电复位
                      // GPIO19 被 SD 卡 MISO 占用, 不能用作 TFT_RST
// TFT_BL: 背光直连电源, 无软件控制。此处不 #undef/#define TFT_BL,
// 保留官方 Mini 屏幕块定义的 TFT_BL=32 (亦由 TFT_eSPI 的
// User_Setup_marauder_mini.h 提供)。对小喵 GPIO32 为空脚,
// .ino 里 digitalWrite(TFT_BL, LOW/HIGH) 无副作用, 编译正常。
#define TOUCH_CS -1

// ---- 5. 屏幕分辨率与方向覆盖 ----
// 1.8" ST7735 横屏有效分辨率: 160 wide x 128 tall
#ifdef DISP_W
  #undef DISP_W
#endif
#ifdef DISP_H
  #undef DISP_H
#endif
#ifdef SCREEN_ORIENTATION
  #undef SCREEN_ORIENTATION
#endif
#ifdef TFT_WIDTH
  #undef TFT_WIDTH
#endif
#ifdef TFT_HEIGHT
  #undef TFT_HEIGHT
#endif
#ifdef HEIGHT_1
  #undef HEIGHT_1
#endif
#ifdef WIDTH_1
  #undef WIDTH_1
#endif
#ifdef SCREEN_WIDTH
  #undef SCREEN_WIDTH
#endif
#ifdef SCREEN_HEIGHT
  #undef SCREEN_HEIGHT
#endif

#define DISP_W  160
#define DISP_H  128
#define SCREEN_ORIENTATION 3   // 3=landscape flipped (修复上下颠倒)
#define TFT_WIDTH   DISP_W     // 应用层横屏宽度
#define TFT_HEIGHT  DISP_H     // 应用层横屏高度
#define HEIGHT_1    DISP_H
#define WIDTH_1     DISP_W
#define SCREEN_WIDTH  DISP_W
#define SCREEN_HEIGHT DISP_H

// ---- 5b. 屏幕布局宏覆盖 (与 160x128 横屏匹配) ----
#ifdef GRAPH_VERT_LIM
  #undef GRAPH_VERT_LIM
#endif
#ifdef STANDARD_FONT_CHAR_LIMIT
  #undef STANDARD_FONT_CHAR_LIMIT
#endif
#ifdef TEXT_HEIGHT
  #undef TEXT_HEIGHT
#endif
#ifdef YMAX
  #undef YMAX
#endif
#ifdef FRAME_X
  #undef FRAME_X
#endif
#ifdef FRAME_Y
  #undef FRAME_Y
#endif
#ifdef FRAME_W
  #undef FRAME_W
#endif
#ifdef FRAME_H
  #undef FRAME_H
#endif
#ifdef REDBUTTON_X
  #undef REDBUTTON_X
#endif
#ifdef REDBUTTON_Y
  #undef REDBUTTON_Y
#endif
#ifdef REDBUTTON_W
  #undef REDBUTTON_W
#endif
#ifdef REDBUTTON_H
  #undef REDBUTTON_H
#endif
#ifdef GREENBUTTON_X
  #undef GREENBUTTON_X
#endif
#ifdef GREENBUTTON_Y
  #undef GREENBUTTON_Y
#endif
#ifdef GREENBUTTON_W
  #undef GREENBUTTON_W
#endif
#ifdef GREENBUTTON_H
  #undef GREENBUTTON_H
#endif
#ifdef STATUS_BAR_WIDTH
  #undef STATUS_BAR_WIDTH
#endif

#define GRAPH_VERT_LIM          (DISP_H / 2 - 1)
#define STANDARD_FONT_CHAR_LIMIT (DISP_W / 6)
#define TEXT_HEIGHT             (DISP_H / 10)
#define YMAX                    DISP_H
#define FRAME_X 80
#define FRAME_Y 48
#define FRAME_W 110
#define FRAME_H 44
#define REDBUTTON_X FRAME_X
#define REDBUTTON_Y FRAME_Y
#define REDBUTTON_W (FRAME_W / 2)
#define REDBUTTON_H FRAME_H
#define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
#define GREENBUTTON_Y FRAME_Y
#define GREENBUTTON_W (FRAME_W / 2)
#define GREENBUTTON_H FRAME_H
#define STATUS_BAR_WIDTH        (DISP_H / 16)

// ---- 6. SD 卡引脚覆盖 ----
// 与屏幕共享 VSPI 总线: SCK/MOSI 相同, MISO=19, CS=22
#ifdef SD_CS
  #undef SD_CS
#endif
#ifdef SD_SCK
  #undef SD_SCK
#endif
#ifdef SD_MISO
  #undef SD_MISO
#endif
#ifdef SD_MOSI
  #undef SD_MOSI
#endif

#define SD_CS   22
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23

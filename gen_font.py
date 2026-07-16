#!/usr/bin/env python3
"""Generate Chinese GFXfont for ESP32 Marauder. Maps Chinese chars to 0x80-0xFF."""

from PIL import Image, ImageDraw, ImageFont
import os

# === Chinese translations (using only 128 chars in font) ===
T = {
    "text0_0": "为件串口备空间...",
    "text0_1": "串口已启动",
    "text0_2": "已检查内存",
    "text0_3": "SD卡已初始化",
    "text0_4": "SD卡初始化失败",
    "text0_5": "已检查电池配置",
    "text0_6": "温度接口已初始化",
    "text0_7": "LED接口已初始化",
    "text0_8": "启动中...",
    "text00": "电量变化: ",
    "text01": "文件已关闭",
    "text02": "打开文件失败 '",
    "text03": "开",
    "text04": "关",
    "text05": "加载",
    "text06": "另存为",
    "text07": "退出",
    "text08": "设置",
    "text09": "返回",
    "text10": "信:",
    "text11": "点退出",
    "text12": "取关",
    "text13": "保存",
    "text14": "定",
    "text15": "正打开更新文件...",
    "text16": "关闭",
    "text17": "失败",
    "text18": "包/s: ",
    "text1_0": "SSID列表",
    "text1_1": "加SSID",
    "text1_2": "SSID: ",
    "text1_3": "凭证:",
    "text1_4": "已禁用",
    "text1_5": "已启用",
    "text1_6": "ESP32",
    "text1_7": "WiFi",
    "text1_8": "BadUSB",
    "text1_9": "设备",
    "text1_10": "通用",
    "text1_11": "更新中...",
    "text1_12": "择法",
    "text1_13": "认定更新",
    "text1_14": "ESP8266更新",
    "text1_15": "固件更新",
    "text1_16": "文",
    "text1_17": "设备信",
    "text1_18": "设置",
    "text1_19": "蓝牙",
    "text1_20": "WiFi嗅探",
    "text1_21": "WiFi攻击",
    "text1_22": "WiFi通用",
    "text1_23": "蓝牙嗅探",
    "text1_24": "蓝牙通用",
    "text1_25": "关闭WiFi",
    "text1_26": "关闭BLE",
    "text1_27": "生成SSID",
    "text1_28": "清除SSID",
    "text1_29": "清除AP",
    "text1_30": "重启",
    "text1_31": "嗅探器",
    "text1_32": "攻击",
    "text1_33": "通用",
    "text1_34": "蓝牙嗅探器",
    "text1_35": "检测读卡器",
    "text1_36": "测试BadUSB",
    "text1_37": "启动Ducky本",
    "text1_38": "图",
    "text1_39": "WEB更新",
    "text1_40": "SD卡更新",
    "text1_41": "ESP8266更新",
    "text1_42": "探测请求嗅探",
    "text1_43": "信标嗅探",
    "text1_44": "解除认证嗅探",
    "text1_45": "数包监",
    "text1_46": "EAPOL扫描",
    "text1_47": "检测Pwnagotchi",
    "text1_48": "检测乐鑫",
    "text1_49": "扫描AP",
    "text1_50": "信标泛洪列表",
    "text1_51": "不定信标泛洪",
    "text1_52": "RickRoll信标",
    "text1_53": "探测请求泛洪",
    "text1_54": "解除认证泛洪",
    "text1_55": "连接WiFi",
    "text1_56": "选择AP",
    "text1_57": "AP克泛洪",
    "text1_58": "原始取",
    "text1_59": "站点嗅探",
    "text1_60": "清除站点",
    "text1_61": "选择站点",
    "text1_62": "定向解除认证",
    "text1_63": "检测Pineapple",
    "text1_64": "检测多SSID",
    "text1_65": "选择探测SSID",
    "text1_66": "GPS",
    "text1_67": "乐SSID信标",
    "text2_0": "错误,不到更新文件",
    "text2_1": "开始SD更新...",
    "text2_2": "错误,更新文件为空",
    "text2_3": "重启中...",
    "text2_4": "无法加载更新文件",
    "text2_5": "文件大小: ",
    "text2_6": "正写入分区...",
    "text2_7": "已写入: ",
    "text2_8": "仅写入: ",
    "text2_9": ". 重试?",
    "text2_10": " 成功",
    "text2_11": "更新完成",
    "text2_12": "更新无法完成",
    "text2_13": "发生错误. 错误#: ",
    "text2_14": "空间小无法OTA",
    "text3_0": "配置更新服务器...",
    "text3_1": "IP地: ",
    "text3_2": "更新: ",
    "text3_3": "已完成数: ",
    "text3_4": "更新成功: ",
    "text3_5": "更新服务器完成",
    "text4_0": " RSSI: ",
    "text4_1": "在读卡器: ",
    "text4_2": "已连接",
    "text4_3": "连接失败",
    "text4_4": "已接",
    "text4_5": "强制PMKID",
    "text4_6": "强制探测",
    "text4_7": "保存PCAP",
    "text4_8": "探测泛洪",
    "text4_9": "正在清除AP...",
    "text4_10": "已清除AP: ",
    "text4_11": "正在清除SSID...",
    "text4_12": "已清除SSID: ",
    "text4_13": "正在生成SSID...",
    "text4_14": "已生成SSID: ",
    "text4_15": "    多SSID数: ",
    "text4_16": "正在关闭WiFi...",
    "text4_17": "WiFi当前未初始化",
    "text4_18": "正在关闭BLE...",
    "text4_19": "BLE当前未初始化",
    "text4_20": "固件: Marauder",
    "text4_21": "本: ",
    "text4_22": "ESP-IDF: ",
    "text4_23": "WSL绕过: 已启用",
    "text4_24": "WSL绕过: 已禁用",
    "text4_25": "站点MAC: ",
    "text4_26": "AP MAC: ",
    "text4_27": "",
    "text4_28": "SD卡: 已连接",
    "text4_29": "SD卡大小: ",
    "text4_30": "SD卡: 未连接",
    "text4_31": "SD卡大小: 0",
    "text4_32": "电池监控: 支持",
    "text4_33": "电量: ",
    "text4_34": "电池监控: 不支持",
    "text4_35": "内温度: ",
    "text4_36": " 检测乐鑫 ",
    "text4_37": " 检测Pwnagotchi ",
    "text4_38": " 信标嗅探器 ",
    "text4_39": " 解除认证嗅探器 ",
    "text4_40": " 探测请求嗅探器 ",
    "text4_41": " 蓝牙嗅探 ",
    "text4_42": " 检测读卡器 ",
    "text4_43": "正扫描蓝牙读卡器",
    "text4_44": " AP扫描 ",
    "text4_45": "正在清除站点...",
    "text4_46": "已清除站点: ",
    "text4_47": "定向解除认证",
    "text4_48": " 检测Pineapple ",
    "text4_49": " 检测多SSID ",
    "loading": "加载中...",
    "wifi_creds_empty": "WiFi凭证为空",
    "returning": "返回中...",
    "connect_fail": "无法连接WiFi",
    "connected": "已连接!",
    "connecting": "连接中",
    "failed_connect": "连接失败",
    "evil_portal": "EVIL PORTAL",
    "scan_ap_sta": "扫描AP/STA",
}

# Use all characters from the translations (T already limited to 128)
all_chars = set()
for v in T.values():
    for ch in v:
        if ord(ch) >= 0x4E00 and ord(ch) <= 0x9FFF:
            all_chars.add(ch)

all_chars = sorted(all_chars)
print(f"Unique Chinese characters: {len(all_chars)}")

if len(all_chars) > 128:
    print(f"ERROR: {len(all_chars)} > 128 chars! Please reduce translations.")
    print(f"Extra chars: {len(all_chars) - 128}")
    exit(1)

# Build mapping
char_to_byte = {ch: 0x80 + i for i, ch in enumerate(all_chars)}

# Find font
font_path = None
for fp in [
    "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
    "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
]:
    if os.path.exists(fp):
        font_path = fp
        break

if font_path is None:
    print("Installing Chinese font...")
    os.system("apt-get update -qq && apt-get install -y -qq fonts-wqy-zenhei 2>/dev/null")
    for fp in ["/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc"]:
        if os.path.exists(fp):
            font_path = fp
            break

assert font_path, "No Chinese font found!"
FONT_SIZE = 16
font = ImageFont.truetype(font_path, FONT_SIZE)
print(f"Using font: {font_path} at {FONT_SIZE}px")

# Render glyphs
glyphs = []
bitmap = bytearray()

for ch in all_chars:
    # Measure
    tmp = Image.new('1', (FONT_SIZE * 2, FONT_SIZE * 2), 0)
    dr = ImageDraw.Draw(tmp)
    bbox = dr.textbbox((0, 0), ch, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    
    if w <= 0 or h <= 0:
        w, h = 4, 1
    
    # Render
    img = Image.new('1', (w, h), 0)
    dr = ImageDraw.Draw(img)
    dr.text((-bbox[0], -bbox[1]), ch, font=font, fill=1)
    data = img.tobytes("raw", "1")
    
    glyphs.append({
        'offset': len(bitmap),
        'w': w, 'h': h,
        'xa': w + 1,
        'xo': bbox[0], 'yo': bbox[1] + 1,
    })
    bitmap.extend(data)

# Calculate yAdvance
tmp = Image.new('1', (FONT_SIZE * 2, FONT_SIZE * 2), 0)
bbox = ImageDraw.Draw(tmp).textbbox((0, 0), "测", font=font)
yAdvance = bbox[3] - bbox[1] + 2

# Generate header
out = '/workspace/esp32_marauder/cn_font.h'
with open(out, 'w') as f:
    f.write('// Auto-generated Chinese font for ESP32 Marauder\n')
    f.write(f'// {len(all_chars)} chars, 0x80-0x{0x80+len(all_chars)-1:02X}, {FONT_SIZE}px\n')
    f.write('#pragma once\n#include <TFT_eSPI.h>\n\n')
    
    # Bitmap
    f.write(f'const uint8_t cn_font_bitmap[{len(bitmap)}] PROGMEM = {{\n')
    for i in range(0, len(bitmap), 16):
        chunk = bitmap[i:i+16]
        line = ', '.join(f'0x{b:02X}' for b in chunk)
        f.write(f'  {line},\n')
    f.write('};\n\n')
    
    # Glyphs
    f.write(f'const GFXglyph cn_font_glyphs[{len(glyphs)}] PROGMEM = {{\n')
    for i, g in enumerate(glyphs):
        f.write(f'  {{{g["offset"]:5d},{g["w"]:3d},{g["h"]:3d},{g["xa"]:3d},{g["xo"]:4d},{g["yo"]:4d}}}, // {all_chars[i]}\n')
    f.write('};\n\n')
    
    # Font struct
    f.write('const GFXfont cn_font PROGMEM = {\n')
    f.write('  (uint8_t*)cn_font_bitmap,\n')
    f.write('  (GFXglyph*)cn_font_glyphs,\n')
    f.write(f'  0x80, 0x{0x80+len(glyphs)-1:02X}, {yAdvance}\n')
    f.write('};\n\n')
    
    # encodeCN function
    f.write('// Encode a UTF-8 Chinese string to our custom byte encoding\n')
    f.write('String encodeCN(const String& input) {\n')
    f.write('  String result; result.reserve(input.length());\n')
    f.write('  for (unsigned int i = 0; i < input.length(); i++) {\n')
    f.write('    unsigned char c = input[i];\n')
    f.write('    if (c < 0x80) { result += (char)c; continue; }\n')
    f.write('    uint16_t unicode = 0;\n')
    f.write('    if ((c & 0xE0) == 0xC0) { unicode = ((c & 0x1F) << 6) | (input[++i] & 0x3F); }\n')
    f.write('    else if ((c & 0xF0) == 0xE0) { unicode = ((c & 0x0F) << 12) | ((input[++i] & 0x3F) << 6) | (input[++i] & 0x3F); }\n')
    f.write('    switch (unicode) {\n')
    for ch in all_chars:
        f.write(f'      case 0x{ord(ch):04X}: result += char(0x{char_to_byte[ch]:02X}); break;\n')
    f.write('    }\n')
    f.write('  }\n')
    f.write('  return result;\n')
    f.write('}\n\n')
    
    # drawCNString
    asc_w = FONT_SIZE // 2
    f.write(f'// Draw mixed ASCII+Chinese string. ASCII uses built-in GLCD font. Chinese uses cn_font.\n')
    f.write(f'void drawCNString(TFT_eSPI& tft, const String& str, int x, int y) {{\n')
    f.write(f'  int cx = x;\n')
    f.write(f'  for (unsigned int i = 0; i < str.length(); i++) {{\n')
    f.write(f'    unsigned char c = str[i];\n')
    f.write(f'    if (c < 0x80) {{ tft.setFreeFont(NULL); tft.setTextSize(1); tft.drawChar(c, cx, y); cx += {asc_w}; }}\n')
    f.write(f'    else {{ tft.setFreeFont(&cn_font); tft.drawChar(c, cx, y); cx += tft.textWidth(String(c)); }}\n')
    f.write(f'  }}\n')
    f.write(f'}}\n\n')
    
    # drawCNCentreString
    f.write(f'void drawCNCentreString(TFT_eSPI& tft, const String& str, int x, int y) {{\n')
    f.write(f'  int tw = 0;\n')
    f.write(f'  for (unsigned int i = 0; i < str.length(); i++) {{\n')
    f.write(f'    unsigned char c = str[i];\n')
    f.write(f'    if (c < 0x80) tw += {asc_w};\n')
    f.write(f'    else {{ tft.setFreeFont(&cn_font); tw += tft.textWidth(String(c)); }}\n')
    f.write(f'  }}\n')
    f.write(f'  drawCNString(tft, str, x - tw / 2, y);\n')
    f.write(f'}}\n')

print(f"Done! Generated {out}")
print(f"Chars: {len(all_chars)}, Bitmap: {len(bitmap)} bytes")

# ============================================================
# Also generate lang_var_cn.h with pre-encoded Chinese strings
# ============================================================
def encode_string(text, char_to_byte):
    """Encode a UTF-8 Chinese string to our custom byte encoding (same as encodeCN)."""
    result = bytearray()
    for ch in text:
        cp = ord(ch)
        if cp < 0x80:
            result.append(cp)
        elif ch in char_to_byte:
            result.append(char_to_byte[ch])
        else:
            # Non-CJK, pass through as-is (shouldn't happen with our data)
            result.append(cp & 0xFF)
    result.append(0)  # null terminator
    return result

# Generate lang_var_cn.h
lang_out = '/workspace/esp32_marauder/lang_var_cn.h'
with open(lang_out, 'w') as f:
    f.write('// Auto-generated Chinese translations for ESP32 Marauder\n')
    f.write('// These strings are pre-encoded; use with drawCNString() / drawCNCentreString()\n')
    f.write('#pragma once\n\n')
    f.write('#include "cn_font.h"\n\n')
    
    # Map T keys to C variable names
    key_map = {
        "text0_0": "text0_0", "text0_1": "text0_1", "text0_2": "text0_2", "text0_3": "text0_3",
        "text0_4": "text0_4", "text0_5": "text0_5", "text0_6": "text0_6", "text0_7": "text0_7",
        "text0_8": "text0_8", "text00": "text00", "text01": "text01", "text02": "text02",
        "text03": "text03", "text04": "text04", "text05": "text05", "text06": "text06",
        "text07": "text07", "text08": "text08", "text09": "text09", "text10": "text10",
        "text11": "text11", "text12": "text12", "text13": "text13", "text14": "text14",
        "text15": "text15", "text16": "text16", "text17": "text17", "text18": "text18",
        "text1_0": "text1_0", "text1_1": "text1_1", "text1_2": "text1_2", "text1_3": "text1_3",
        "text1_4": "text1_4", "text1_5": "text1_5", "text1_6": "text1_6", "text1_7": "text1_7",
        "text1_8": "text1_8", "text1_9": "text1_9", "text1_10": "text1_10", "text1_11": "text1_11",
        "text1_12": "text1_12", "text1_13": "text1_13", "text1_14": "text1_14", "text1_15": "text1_15",
        "text1_16": "text1_16", "text1_17": "text1_17", "text1_18": "text1_18", "text1_19": "text1_19",
        "text1_20": "text1_20", "text1_21": "text1_21", "text1_22": "text1_22", "text1_23": "text1_23",
        "text1_24": "text1_24", "text1_25": "text1_25", "text1_26": "text1_26", "text1_27": "text1_27",
        "text1_28": "text1_28", "text1_29": "text1_29", "text1_30": "text1_30", "text1_31": "text1_31",
        "text1_32": "text1_32", "text1_33": "text1_33", "text1_34": "text1_34", "text1_35": "text1_35",
        "text1_36": "text1_36", "text1_37": "text1_37", "text1_38": "text1_38", "text1_39": "text1_39",
        "text1_40": "text1_40", "text1_41": "text1_41", "text1_42": "text1_42", "text1_43": "text1_43",
        "text1_44": "text1_44", "text1_45": "text1_45", "text1_46": "text1_46", "text1_47": "text1_47",
        "text1_48": "text1_48", "text1_49": "text1_49", "text1_50": "text1_50", "text1_51": "text1_51",
        "text1_52": "text1_52", "text1_53": "text1_53", "text1_54": "text1_54", "text1_55": "text1_55",
        "text1_56": "text1_56", "text1_57": "text1_57", "text1_58": "text1_58", "text1_59": "text1_59",
        "text1_60": "text1_60", "text1_61": "text1_61", "text1_62": "text1_62", "text1_63": "text1_63",
        "text1_64": "text1_64", "text1_65": "text1_65", "text1_66": "text1_66", "text1_67": "text1_67",
        "text2_0": "text2_0", "text2_1": "text2_1", "text2_2": "text2_2", "text2_3": "text2_3",
        "text2_4": "text2_4", "text2_5": "text2_5", "text2_6": "text2_6", "text2_7": "text2_7",
        "text2_8": "text2_8", "text2_9": "text2_9", "text2_10": "text2_10", "text2_11": "text2_11",
        "text2_12": "text2_12", "text2_13": "text2_13", "text2_14": "text2_14",
        "text3_0": "text3_0", "text3_1": "text3_1", "text3_2": "text3_2", "text3_3": "text3_3",
        "text3_4": "text3_4", "text3_5": "text3_5",
        "text4_0": "text4_0", "text4_1": "text4_1", "text4_2": "text4_2", "text4_3": "text4_3",
        "text4_4": "text4_4", "text4_5": "text4_5", "text4_6": "text4_6", "text4_7": "text4_7", "text4_8": "text4_8",
        "text4_9": "text4_9", "text4_10": "text4_10", "text4_11": "text4_11", "text4_12": "text4_12",
        "text4_13": "text4_13", "text4_14": "text4_14", "text4_15": "text4_15", "text4_16": "text4_16",
        "text4_17": "text4_17", "text4_18": "text4_18", "text4_19": "text4_19", "text4_20": "text4_20",
        "text4_21": "text4_21", "text4_22": "text4_22", "text4_23": "text4_23", "text4_24": "text4_24",
        "text4_25": "text4_25", "text4_26": "text4_26", "text4_27": "text4_27", "text4_28": "text4_28", "text4_29": "text4_29",
        "text4_30": "text4_30", "text4_31": "text4_31", "text4_32": "text4_32", "text4_33": "text4_33",
        "text4_34": "text4_34", "text4_35": "text4_35", "text4_36": "text4_36", "text4_37": "text4_37",
        "text4_38": "text4_38", "text4_39": "text4_39", "text4_40": "text4_40", "text4_41": "text4_41",
        "text4_42": "text4_42", "text4_43": "text4_43", "text4_44": "text4_44", "text4_45": "text4_45",
        "text4_46": "text4_46", "text4_47": "text4_47", "text4_48": "text4_48", "text4_49": "text4_49",
        "loading": "cn_loading", "wifi_creds_empty": "cn_wifi_creds_empty",
        "returning": "cn_returning", "connect_fail": "cn_connect_fail",
        "connected": "cn_connected", "connecting": "cn_connecting",
        "failed_connect": "cn_failed_connect", "evil_portal": "cn_evil_portal",
        "scan_ap_sta": "cn_scan_ap_sta",
    }
    
    # Section comments
    sections = {
        "text0_0": "// Starting window texts",
        "text00": "// Single library (action) texts / Often used",
        "text1_0": "// MenuFunctions.cpp texts",
        "text2_0": "// SDInterface.cpp texts",
        "text3_0": "// Web.cpp texts",
        "text4_0": "// WiFiScan.cpp texts",
        "loading": "// Additional strings",
    }
    
    last_section = None
    for t_key in T:
        c_name = key_map.get(t_key, t_key)
        encoded = encode_string(T[t_key], char_to_byte)
        
        # Section comment
        for section_key, comment in sections.items():
            if t_key == section_key:
                f.write(f'\n{comment}\n')
                break
        
        # Write as C byte array with PROGMEM
        f.write(f'static const uint8_t {c_name}[] PROGMEM = {{')
        for i, b in enumerate(encoded):
            if i % 16 == 0:
                f.write('\n  ')
            f.write(f'0x{b:02X}, ')
        f.write('\n};\n\n')

print(f"Done! Generated {lang_out}")
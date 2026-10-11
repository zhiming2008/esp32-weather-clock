/**
 * config.h —— 全局硬件引脚与系统常量
 *
 * 所有与"这块板子怎么接线"相关的定义都集中在这里，
 * 换硬件时只需要改这一个文件。
 */
#pragma once

#include <Arduino.h>

// ==================== 硬件引脚 ====================

// --- TFT 屏（SPI 接口，ILI9488 驱动，320×480 横屏）---
// 注意：屏幕的 SDO(MISO) 接到 MISO 时，同一条 SPI 总线上不能再挂
// 会被 CS 拉高后回灌电平的器件，否则读卡/触摸会出错。
#define PIN_TFT_MOSI   23
#define PIN_TFT_SCLK   18
#define PIN_TFT_MISO   19
#define PIN_TFT_CS      5
#define PIN_TFT_DC      2
#define PIN_TFT_RST     4

// 背光未由 GPIO 驱动（模块自带限流电阻，直接接 3.3V 常亮）。
// 如果你的屏有独立背光脚，把下面的宏改成实际 GPIO 并在
// Display::begin() 里调用 pinMode + digitalWrite 即可。
#define PIN_TFT_BACKLIGHT (-1)

// --- 温湿度传感器 ---
#define PIN_DHT        17

// --- 板载状态 LED（ESP32-DevKitC 板载 RGB LED 的数据脚）---
#define PIN_LED        2   // 注意：板载 RGB LED 占用 GPIO2，
                            // 若与 TFT_DC 冲突请改用外部 LED
#define LED_ON         HIGH
#define LED_OFF        LOW

// --- BOOT 键：长按清除已保存的 WiFi 凭据 ---
#define PIN_BOOT        0
#define BOOT_HOLD_MS  5000   // 判定为"长按"的时长

// ==================== 系统常量 ====================

#define WIFI_CONNECT_TIMEOUT_S   15
#define WIFI_HOSTNAME     "esp32-weather-clock"
#define WIFI_AP_SSID      "ESP32-Weather-Setup"
#define WIFI_AP_PASSWORD  "12345678"     // 至少 8 位

#define SERIAL_BAUD      115200

// DHT11 采样周期。数据手册要求两次读取间隔 ≥ 2s，
// 靠 loop() 里的 delay 是靠不住的，这里用时间戳硬性约束。
#define SENSOR_INTERVAL_MS  2000

// 天气刷新周期（分钟）。和风天气免费版的更新频率约 10 分钟。
#define WEATHER_REFRESH_MINUTES 10

// 屏幕旋转：1 = 横屏 480×320，0 = 竖屏 320×480
#define SCREEN_ROTATION      1

// ==================== 屏幕布局 ====================
namespace Layout {

// 分区：左侧 80px 为时间/日期区，右侧为天气与传感器区
constexpr int16_t DIVIDER_X        = 80;    // 竖分隔线
constexpr int16_t HLINE_Y          = 160;   // 横分隔线（右半区）

constexpr int16_t HOUR_X           = 85;
constexpr int16_t HOUR_Y           = 20;
constexpr int16_t COLON_X          = 135;
constexpr int16_t MIN_X            = 160;
constexpr int16_t MIN_Y            = 20;
constexpr int16_t SEC_X            = 240;
constexpr int16_t SEC_Y            = 30;

constexpr int16_t MONTH_X          = 100;
constexpr int16_t DAY_X            = 160;
constexpr int16_t DATE_Y           = 100;
constexpr int16_t WEEKDAY_X        = 240;

// 局部刷新矩形（只擦需要重画的那块，避免整屏闪烁）
constexpr int16_t SEC_BOX_X        = 240;
constexpr int16_t SEC_BOX_Y        = 0;
constexpr int16_t SEC_BOX_W        = 79;
constexpr int16_t SEC_BOX_H        = 79;

constexpr int16_t MIN_BOX_X        = 160;
constexpr int16_t MIN_BOX_Y        = 0;
constexpr int16_t MIN_BOX_W        = 79;
constexpr int16_t MIN_BOX_H        = 79;

constexpr int16_t WEEKDAY_BOX_X    = 160;
constexpr int16_t WEEKDAY_BOX_Y    = 80;
constexpr int16_t WEEKDAY_BOX_W    = 159;
constexpr int16_t WEEKDAY_BOX_H    = 79;

constexpr int16_t DATE_BOX_X       = 80;
constexpr int16_t DATE_BOX_Y       = 80;
constexpr int16_t DATE_BOX_W       = 160;
constexpr int16_t DATE_BOX_H       = 160;

// 右侧传感器读数块（局部刷新矩形）
constexpr int16_t SENSOR_BOX_X     = 321;
constexpr int16_t SENSOR_BOX_Y     = 0;
constexpr int16_t SENSOR_BOX_W     = 159;
constexpr int16_t SENSOR_BOX_H     = 159;

// 右侧天气文字块
constexpr int16_t WEATHER_X1       = 85;
constexpr int16_t WEATHER_Y1       = 180;   // 天气现象
constexpr int16_t WEATHER_Y2       = 260;   // 温度
constexpr int16_t FEEL_X           = 260;
constexpr int16_t FEEL_Y           = 170;   // 体感温度
constexpr int16_t HUMID_X          = 260;
constexpr int16_t HUMID_Y          = 223;   // 相对湿度
constexpr int16_t WIND_X           = 260;
constexpr int16_t WIND_Y           = 276;   // 风力等级

}  // namespace Layout

// ==================== 配色 ====================
namespace Palette {
constexpr uint16_t Background = TFT_BLACK;
constexpr uint16_t Text       = TFT_WHITE;
constexpr uint16_t Accent     = TFT_CYAN;    // 时分
constexpr uint16_t Secondary  = TFT_YELLOW;  // 月日
constexpr uint16_t Alert      = TFT_RED;     // 秒
}  // namespace Palette

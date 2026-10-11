/**
 * Display.h —— 屏幕绘制
 *
 * 所有坐标常量集中在 include/config.h 的 Layout 命名空间里，
 * 换屏幕尺寸时只改那一处。
 *
 * 刷新策略：整屏重画会有明显闪烁，所以按"变化频率"分块局部擦除：
 *   · 秒   —— 每秒擦 79×79
 *   · 分   —— 每分钟擦一次
 *   · 星期 —— 每小时擦一次
 *   · 日期 —— 每天零点擦一次
 * 原代码里这几块的擦除条件写错了（|| 与 && 混用没有括号，
 * 导致"星期区永远不会被擦掉"），这里按正确的层级重写。
 */
#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "../modules/Clock.h"
#include "../modules/Sensor.h"
#include "../modules/Weather.h"

class Display {
 public:
  void begin();

  /// 画分隔线（时间区 / 天气区）
  void drawFrame();

  /// 绘制时间与日期，内部按需局部刷新
  void drawClock(const DateTimeInfo& t);

  /// 绘制天气区块（天气现象、温度、体感、湿度、风力）
  void drawWeather(const WeatherNow& w);

  /// 绘制右上角的本地传感器读数
  void drawClimate(const ClimateReading& c);

  /// 全屏提示（配网等待、连接中、错误等）
  void showStatus(const String& line1, const String& line2 = String());

  TFT_eSPI& screen() { return _tft; }

 private:
  static TFT_eSPI _tft;   // TFT_eSPI 建议使用单例实例
};

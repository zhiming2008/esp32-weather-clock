/**
 * Weather.h —— 和风天气（QWeather）实时天气 + 3 日预报
 *
 * 关于原实现的两处改动：
 *  1. 去掉了 gzip。原代码 URL 里写 gzip=n（不要压缩），
 *     请求头却发了 Accept-Encoding: gzip，然后再用 ArduinoUZlib
 *     手动解压并 free() —— 自相矛盾，还白白引入了第三方库和
 *     一块 3KB 静态缓冲区。这里直接 HTTPClient 拿字符串解析。
 *  2. 修好了 doUpdateForecast 的越界 bug。原函数签名收单个结构体指针，
 *     函数体里却当数组用 data[i]（i 0..2），调用者传数组指针才行，
 *     签名与实现不一致。这里签名直接写成数组。
 *
 * API 文档：https://devapi.qweather.com/docs/api/
 */
#pragma once

#include <Arduino.h>
#include "config.h"

struct WeatherNow {
  String text       = "-";   // 天气现象，如「晴」
  String temp       = "-";   // 温度 ℃
  String feelsLike  = "-";   // 体感温度 ℃
  String humidity   = "-";   // 相对湿度 %
  String windScale  = "-";   // 风力等级
  String iconCode   = "";    // 官方图标编码
  bool   valid      = false;
};

struct DailyForecast {
  String date;      // MM-DD
  String tempMin;   // 最低温 ℃
  String tempMax;   // 最高温 ℃
  String iconDay;   // 白天图标编码
};

class Weather {
 public:
  explicit Weather(const char* apiKey, const char* cityId)
      : _apiKey(apiKey), _cityId(cityId) {}

  /// 拉取实时天气。内部有失败重试，单次失败不阻塞主循环太久。
  bool updateNow(WeatherNow& out);

  /// 拉取 3 日预报（可选功能，失败不影响主显示）
  bool updateForecast(DailyForecast out[3]);

  /// 把官方图标编码映射成单字符标记。
  /// 这些字符是自制字库里预留的图标字模槽位（见 fonts/README.md）。
  static char iconSlot(const String& code);

 private:
  bool httpGet(const String& path, String& bodyOut);

  const char* _apiKey;
  const char* _cityId;
  unsigned long _lastRetryMs = 0;
  uint8_t      _failStreak  = 0;
};

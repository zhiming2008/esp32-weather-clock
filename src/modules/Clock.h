/**
 * Clock.h —— NTP 网络对时 + 本地时间
 *
 * 原代码用的是 NTPClient::setTimeOffset() + gmtime()，
 * 那个偏移量是"每秒猜一次"的做法：网络抖动时会出现时间跳变，
 * 而且 gmtime() 返回的是 UTC，自己再加偏移很容易多减/少减一次。
 *
 * 这里换成 ESP32 标准的 configTime()：由系统时区自动处理 UTC↔本地时间，
 * 拿到的是本地的年月日时分秒。
 */
#pragma once

#include <Arduino.h>

struct DateTimeInfo {
  int      year   = 2024;
  int      month  = 1;
  int      day    = 1;
  int      hour   = 0;
  int      minute = 0;
  int      second = 0;
  int      weekday = 0;   // 0 = 周日
  bool     valid  = false;
};

class Clock {
 public:
  void begin();

  /// 取当前本地时间。未同步成功时 valid=false。
  DateTimeInfo now();

  /// 网络时间是否已同步
  bool synced() const { return _synced; }

 private:
  bool _synced = false;
};

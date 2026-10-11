#include "Clock.h"

#include <stdlib.h>   // setenv
#include <time.h>
#include <sys/time.h>

namespace {
// 国家授时中心（北京时间，UTC+8 无夏令时）
constexpr char kNtpServer1[] = "ntp.ntsc.ac.cn";
constexpr char kNtpServer2[] = "cn.pool.ntp.org";
constexpr char kTz[]        = "CST-8";   // POSIX TZ 写法：西八区 = UTC+8
}  // namespace

void Clock::begin() {
  // configTime 会启动一次 SNTP 同步，之后系统时间由 lwIP 维护，
  // 比每秒自己算偏移稳得多。
  configTime(0, 0, kNtpServer1, kNtpServer2);
  setenv("TZ", kTz, 1);
  tzset();
  _synced = false;
  Serial.println(F("[Clock] NTP 同步已启动"));
}

DateTimeInfo Clock::now() {
  DateTimeInfo t;
  time_t epoch = time(nullptr);   // time() 在同步完成后才有意义

  if (epoch < 1700000000) {       // 明显还没同步（2023-11 之前）
    t.valid = false;
    return t;
  }

  struct tm tmNow;
  localtime_r(&epoch, &tmNow);

  t.year    = tmNow.tm_year + 1900;
  t.month   = tmNow.tm_mon  + 1;
  t.day     = tmNow.tm_mday;
  t.hour    = tmNow.tm_hour;
  t.minute  = tmNow.tm_min;
  t.second  = tmNow.tm_sec;
  t.weekday = tmNow.tm_wday;
  t.valid   = true;

  if (!_synced) {
    _synced = true;
    Serial.printf("[Clock] 时间已同步：%04d-%02d-%02d %02d:%02d:%02d\n",
                  t.year, t.month, t.day, t.hour, t.minute, t.second);
  }
  return t;
}

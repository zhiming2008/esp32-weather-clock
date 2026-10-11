/**
 * Sensor.h —— DHT11 温湿度采集
 *
 * DHT11 的协议比较别扭：两次读取之间必须间隔 ≥ 2 秒，
 * 否则会返回上一次的旧值甚至 NaN。原代码是在 loop() 里靠 delay(30)
 * 顶着读，等于没有遵守这个约束。这里改成时间戳驱动的非阻塞采样。
 */
#pragma once

#include <Arduino.h>
#include "config.h"

struct ClimateReading {
  float temperature = 0.0f;   // ℃
  float humidity    = 0.0f;   // %
  bool  valid       = false;  // false 表示本次采样失败或还没采过
};

class Sensor {
 public:
  void begin();
  /// 到期才真正读一次硬件。返回 true 表示 _reading 里有新的有效数据。
  bool update();
  const ClimateReading& reading() const { return _reading_; }

 private:
  ClimateReading _reading_;
  uint32_t _lastSampleMs = 0;
  bool     _hasSample    = false;
};

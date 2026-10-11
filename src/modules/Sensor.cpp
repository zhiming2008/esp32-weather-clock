#include "Sensor.h"

#include <DHT.h>

namespace {
// DHT11 与 DHT22 的取值范围和精度不同，型号在这里改
constexpr uint8_t kSensorType = DHT11;
DHT g_dht(PIN_DHT, kSensorType);
}  // namespace

void Sensor::begin() {
  g_dht.begin();
  _lastSampleMs = 0;
  _hasSample    = false;
  _reading_     = ClimateReading{};
}

bool Sensor::update() {
  // 首次调用立即采一次；之后每 SENSOR_INTERVAL_MS 采一次。
  // 用 millis() 差值判断，wrap-around（49 天溢出）天然安全。
  if (_hasSample &&
      (static_cast<uint32_t>(millis() - _lastSampleMs) < SENSOR_INTERVAL_MS)) {
    return false;
  }

  const float rawT = g_dht.readTemperature();
  const float rawH = g_dht.readHumidity();
  _lastSampleMs = millis();
  _hasSample    = true;

  // 读取失败时 DHT 库返回 NaN，必须挡掉，
  // 否则 NaN 会被 String() 渲染成字符串 "nan" 显示到屏上。
  if (isnan(rawT) || isnan(rawH)) {
    _reading_.valid = false;
    Serial.println(F("[Sensor] DHT 读取失败（NaN），跳过本次采样"));
    return false;
  }

  _reading_.temperature = round(rawT * 10.0f) / 10.0f;
  _reading_.humidity    = round(rawH * 10.0f) / 10.0f;
  _reading_.valid       = true;
  return true;
}

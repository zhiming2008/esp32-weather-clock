/**
 * main.cpp —— ESP32 联网天气时钟
 *
 * 调度思路：主循环里不做任何长阻塞，每个模块自己管自己的节奏。
 *   · NetManager  每轮都跑，维护连接 + 响应配网页面
 *   · Sensor      距上次采样 ≥2s 才读一次 DHT11
 *   · Clock       每轮取一次本地时间，Display 按需局部刷新
 *   · Weather     距上次刷新 ≥10 分钟才请求一次 HTTPS
 *   · BOOT 键     轮询式检测长按，不占用 delay
 *
 * 硬件接线见 docs/wiring.md，引脚见 include/config.h。
 */

#include <Arduino.h>

#include "config.h"

// 未配置密钥时用占位符兜底，保证 clone 下来也能编译通过
// （天气会显示"获取失败"，但整机逻辑不受影响）
#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "未找到 include/secrets.h —— 正在使用占位符，请复制 secrets.example.h 并填入自己的 API Key"
#define HEFENG_API_KEY "YOUR_HEFENG_API_KEY_HERE"
#define CITY_ID       "101270401"
#endif

#include "modules/Clock.h"
#include "modules/Display.h"
#include "modules/NetManager.h"
#include "modules/Sensor.h"
#include "modules/Weather.h"

namespace {

enum class RunState { kProvisioning, kRunning };

Sensor      g_sensor;
Clock       g_clock;
Display     g_display;
NetManager  g_net;
Weather     g_weather(HEFENG_API_KEY, CITY_ID);

WeatherNow  g_now;
RunState    g_state      = RunState::kRunning;
uint32_t    g_lastWeatherMs = 0;
uint32_t    g_lastClockDrawMs = 0;
bool        g_bootPressed = false;
uint32_t    g_bootDownMs  = 0;

/// 拉一次天气并刷新屏幕。首次调用和网络恢复后调用。
void refreshWeather(bool force) {
  const uint32_t interval =
      static_cast<uint32_t>(WEATHER_REFRESH_MINUTES) * 60000UL;
  if (!force && static_cast<uint32_t>(millis() - g_lastWeatherMs) < interval) {
    return;
  }
  g_lastWeatherMs = millis();

  if (g_weather.updateNow(g_now)) {
    Serial.printf("[Main] 天气已更新：%s %s℃ 体感%s℃\n",
                  g_now.text.c_str(), g_now.temp.c_str(),
                  g_now.feelsLike.c_str());
  } else {
    Serial.println(F("[Main] 天气获取失败，保留上一次数据"));
  }
  g_display.drawWeather(g_now);
}

/// 非阻塞检测 BOOT 键长按：清空 WiFi 凭据
void serviceBootButton() {
  const bool pressed = (digitalRead(PIN_BOOT) == LOW);   // BOOT 键按下为低

  if (pressed && !g_bootPressed) {
    g_bootPressed = true;
    g_bootDownMs  = millis();
  } else if (!pressed) {
    g_bootPressed = false;
  }

  if (g_bootPressed &&
      static_cast<uint32_t>(millis() - g_bootDownMs) > BOOT_HOLD_MS) {
    Serial.println(F("[Main] 检测到长按 BOOT 键，清除 WiFi 凭据"));
    g_display.showStatus("正在清除 WiFi", "设备即将重启");
    delay(1500);
    NetManager::forget();
    ESP.restart();
  }
}

}  // namespace

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println(F("\n==================== ESP32 联网天气时钟 ===================="));

  g_display.begin();
  g_sensor.begin();

  pinMode(PIN_BOOT, INPUT_PULLUP);

  g_net.begin();     // 阻塞最多 WIFI_CONNECT_TIMEOUT_S 秒

  if (!g_net.connected()) {
    g_state = RunState::kProvisioning;
    g_display.showStatus("正在配置 WiFi", "请连接热点 " WIFI_AP_SSID);
    Serial.printf("[Main] 等待配网，浏览器打开 http://%s\n",
                  g_net.softApIp().c_str());
    return;
  }

  g_state = RunState::kRunning;
  g_clock.begin();

  // 先给一句"正在取数据"，因为 HTTPS 请求是阻塞的（最长 8s 超时），
  // 否则上电后会有几秒黑屏。之后 drawWeather 会把结果画上去，
  // 最后补画分隔线（showStatus 是全屏清屏，会把线擦掉）。
  g_display.showStatus("正在获取天气...");
  refreshWeather(true);
  g_display.drawFrame();
}

void loop() {
  serviceBootButton();
  g_net.loop();

  if (g_state == RunState::kProvisioning) {
    return;   // 配网期间不跑其它逻辑，NetManager::loop() 会一直响应网页
  }

  // --- 温湿度：到期才采样，采到新值才重绘 ---
  if (g_sensor.update()) {
    g_display.drawClimate(g_sensor.reading());
  }

  // --- 时间：每秒刷新一次足够，Display 内部做局部擦除 ---
  const DateTimeInfo t = g_clock.now();
  if (t.valid &&
      static_cast<uint32_t>(millis() - g_lastClockDrawMs) >= 1000) {
    g_lastClockDrawMs = millis();
    g_display.drawClock(t);
  }

  // --- 网络恢复后补拉一次天气 ---
  refreshWeather(false);

  delay(20);
}

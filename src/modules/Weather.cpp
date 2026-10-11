#include "Weather.h"

#include <memory>    // std::unique_ptr

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

namespace {

constexpr char kHost[] = "devapi.qweather.com";

// 连续失败后的退避：1min → 2min → 4min → 8min 封顶，
// 避免网络断了以后每分钟都去撞一次 HTTPS 握手。
constexpr unsigned long kBackoffMaxMs = 8UL * 60UL * 1000UL;

String twoDigits(int v) {
  return (v < 10 ? "0" : "") + String(v);
}

}  // namespace

bool Weather::httpGet(const String& path, String& bodyOut) {
  const String url = String("https://") + kHost + path;

  // 一次 TLS 上下文只服务一次请求，这里用 unique_ptr 保证异常路径也释放。
  // setInsecure() 是因为 ESP32 框架里没有预置和风天气的根证书，
  // 由 API Key 做鉴权。生产环境建议把根证书烧进 NVS 后改成 setCACert()。
  std::unique_ptr<WiFiClientSecure> tls(new WiFiClientSecure);
  tls->setInsecure();

  HTTPClient http;
  if (!http.begin(*tls, url)) {
    Serial.printf("[Weather] 无法建立连接：%s\n", url.c_str());
    return false;
  }
  http.setTimeout(8000);
  http.setUserAgent("esp32-weather-clock/1.0");

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[Weather] HTTP %d (%s)\n", code,
                  http.errorToString(code).c_str());
    http.end();
    return false;
  }

  bodyOut = http.getString();
  http.end();
  return true;
}

bool Weather::updateNow(WeatherNow& out) {
  // 退避：失败次数越多，下次重试越晚
  if (_failStreak > 0) {
    const unsigned long backoff = 1000UL * (1UL << (_failStreak > 4 ? 4 : _failStreak));
    if (static_cast<unsigned long>(millis() - _lastRetryMs) < backoff) {
      return false;
    }
  }

  String body;
  const String path = "/v7/weather/now?location=" + String(_cityId) +
                      "&key=" + String(_apiKey);
  if (!httpGet(path, body)) {
    ++_failStreak;
    _lastRetryMs = millis();
    return false;
  }

  // now 接口的响应很小，1KB 文档足够
  StaticJsonDocument<1024> doc;
  if (deserializeJson(doc, body)) {
    Serial.println(F("[Weather] JSON 解析失败"));
    ++_failStreak;
    _lastRetryMs = millis();
    return false;
  }

  // 和风会返回一个业务 code 字段，HTTP 200 不代表业务成功
  const char* code = doc["code"] | "unknown";
  if (String(code) != "200") {
    Serial.printf("[Weather] 业务错误 code=%s\n", code);
    ++_failStreak;
    _lastRetryMs = millis();
    return false;
  }

  JsonObject now = doc["now"].as<JsonObject>();
  out.text      = now["text"]      | "-";
  out.temp      = now["temp"]      | "-";
  out.feelsLike = now["feelsLike"] | "-";
  out.humidity  = now["humidity"]  | "-";
  out.windScale = now["windScale"] | "-";
  out.iconCode  = now["icon"]      | "";
  out.valid     = true;

  _failStreak  = 0;
  _lastRetryMs = millis();
  return true;
}

bool Weather::updateForecast(DailyForecast out[3]) {
  for (int i = 0; i < 3; ++i) {   // 参数是数组指针，这里按数组用是正确的
    out[i] = DailyForecast{};
  }

  String body;
  const String path = "/v7/weather/3d?location=" + String(_cityId) +
                      "&key=" + String(_apiKey);
  if (!httpGet(path, body)) return false;

  // daily 里有 3~5 天、每天十几个字段，文档给大一点
  StaticJsonDocument<4096> doc;
  if (deserializeJson(doc, body)) return false;

  JsonArray daily = doc["daily"].as<JsonArray>();
  const int n = static_cast<int>(daily.size());
  const int take = (n < 3) ? n : 3;
  for (int i = 0; i < take; ++i) {
    JsonObject d = daily[i].as<JsonObject>();
    const String date = d["fxDate"] | "";
    // fxDate 形如 2024-08-24，只留 MM-DD
    out[i].date    = (date.length() >= 10) ? date.substring(5, 10) : date;
    out[i].tempMin = d["tempMin"] | "-";
    out[i].tempMax = d["tempMax"] | "-";
    out[i].iconDay = d["iconDay"] | "";
  }
  return true;
}

char Weather::iconSlot(const String& code) {
  // 官方图标编码 → 单字符槽位。
  // 映射表来自和风天气图标文档，字符本身是自制字库里的字模索引。
  if (code == "100" || code == "150" || code == "9006") return 'B';  // 晴
  if (code == "101")                                 return 'Y';  // 多云
  if (code == "102")                                 return 'N';  // 少云
  if (code == "103" || code == "153")                return 'H';  // 晴间多云
  if (code == "104" || code == "154")                return 'D';  // 阴
  if (code == "300" || code == "301")                return 'T';  // 阵雨
  if (code == "302" || code == "303")                return 'P';  // 雷阵雨
  if (code == "304" || code == "313" || code == "404" ||
      code == "405" || code == "406")                return 'X';  // 冻雨/雨夹雪
  if (code == "305" || code == "308" || code == "309" ||
      code == "314" || code == "399")                return 'Q';  // 小雨/毛毛雨
  if (code == "306" || code == "307" || code == "310" ||
      code == "311" || code == "312" || code == "315" ||
      code == "316" || code == "317" || code == "318") return 'R'; // 大到暴雨
  if (code == "400" || code == "408")                return 'U';  // 小雪
  if (code == "401" || code == "402" || code == "403" ||
      code == "409" || code == "410")                return 'W';  // 中到大雪
  if (code == "407")                                 return 'V';  // 阵雪
  if (code == "499" || code == "901")                return 'G';  // 雪/冷
  if (code == "500")                                 return 'E';  // 薄雾
  if (code == "501" || code == "509" ||
      code == "510" || code == "514" ||
      code == "515")                                 return 'M';  // 雾
  if (code == "502" || code == "511" ||
      code == "512" || code == "513")                return 'L';  // 霾
  if (code == "503" || code == "504" ||
      code == "507" || code == "508")                return 'F';  // 扬沙
  if (code == "999")                                 return ')';  // 未知
  if (code == "213")                                 return 'O';  // 阵雪夹杂雹
  if (code.startsWith("2") && code.length() == 3)    return 'S';  // 200~212 雾淞
  return ')';
}

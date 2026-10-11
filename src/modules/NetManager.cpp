#include "NetManager.h"

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_wifi.h>

namespace {

DNSServer  g_dns;
WebServer  g_server(80);
bool       g_serverStarted = false;
bool       g_apStarted     = false;

constexpr uint32_t kReconnectIntervalMs = 15000;   // 断线后多久重试一次
constexpr uint32_t kApStickyMs         = 180000;  // 配网模式最多停留 3 分钟

String htmlEscape(const String& s) {
  String out = s;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  return out;
}

}  // namespace

// ============================ 公开接口 ============================

void NetManager::begin() {
  Serial.printf("[Net] 正在连接已保存的 WiFi（超时 %d 秒）…\n",
                WIFI_CONNECT_TIMEOUT_S);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(DEVICE_HOSTNAME);
  WiFi.setAutoReconnect(true);

  const uint32_t startMs = millis();
  WiFi.begin();   // 不传参数 = 用 NVS 里保存的凭据

  while (WiFi.status() != WL_CONNECTED) {
    if (static_cast<uint32_t>(millis() - startMs) >
        static_cast<uint32_t>(WIFI_CONNECT_TIMEOUT_S) * 1000UL) {
      break;
    }
    delay(100);
  }

  if (connected()) {
    _connected = true;
    Serial.printf("[Net] 已连接：%s  IP=%s\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    MDNS.begin(DEVICE_HOSTNAME);
    MDNS.addService("http", "tcp", 80);
    setLed(true);
  } else {
    Serial.println(F("[Net] 无可用凭据或连接失败，进入配网模式"));
    setLed(false);
    startAccessPoint();
  }
}

void NetManager::loop() {
  if (_inApMode) {
    // 停留超过 3 分钟还没配网成功就退出 AP，避免一直占着
    if (static_cast<uint32_t>(millis() - _lastReconnectMs) > kApStickyMs) {
      Serial.println(F("[Net] 配网超时，重启"));
      ESP.restart();
    }
    if (g_serverStarted) g_server.handleClient();
    return;
  }

  // 已连接状态下做掉线检测
  if (WiFi.status() != WL_CONNECTED) {
    if (_connected) {
      Serial.println(F("[Net] 连接断开"));
      _connected = false;
      setLed(false);
    }
    if (static_cast<uint32_t>(millis() - _lastReconnectMs) > kReconnectIntervalMs) {
      _lastReconnectMs = millis();
      Serial.println(F("[Net] 尝试重连…"));
      WiFi.reconnect();
    }
  } else if (!_connected) {
    _connected = true;
    Serial.printf("[Net] 已重新连接：%s\n", WiFi.SSID().c_str());
    setLed(true);
  }
}

bool NetManager::connected() const {
  return WiFi.status() == WL_CONNECTED;
}

String NetManager::softApIp() const {
  return WiFi.softAPIP().toString();
}

void NetManager::forget() {
  Serial.println(F("[Net] 清除已保存的 WiFi 凭据"));
  WiFi.disconnect(true);   // 带 wifioff=true，顺带把射频关掉，避免占着 NVS
  delay(500);
  esp_wifi_restore();      // 把 NVS 里的 wifi 配置恢复出厂
  delay(500);
}

// ============================ 内部实现 ============================

void NetManager::startAccessPoint() {
  _inApMode         = true;
  _lastReconnectMs = millis();   // 复用这个时间戳当作"进入 AP 的时刻"

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
  g_dns.start(53, WIFI_AP_SSID, WiFi.softAPIP());

  startWebServer();
  g_apStarted = true;

  Serial.printf("[Net] 配网热点已开启：%s\n", WIFI_AP_SSID);
  Serial.printf("[Net] 请用手机连接该热点，浏览器打开 http://%s\n",
                WiFi.softAPIP().toString().c_str());
}

void NetManager::startWebServer() {
  if (g_serverStarted) return;
  g_server.on("/",            HTTP_GET,  [this]() { handleRoot(); });
  g_server.on("/save",        HTTP_POST, [this]() { handleSubmit(); });
  g_server.onNotFound([this]() { handleNotFound(); });
  g_server.begin();
  g_serverStarted = true;
}

void NetManager::stopWebServer() {
  if (!g_serverStarted) return;
  g_server.stop();
  g_serverStarted = false;
}

String NetManager::scanNetworksHtml() {
  const int n = WiFi.scanNetworks();
  String options;
  for (int i = 0; i < n; ++i) {
    options += "<option value=\"" + htmlEscape(WiFi.SSID(i)) + "\">" +
               htmlEscape(WiFi.SSID(i)) + " (" +
               String(WiFi.RSSI(i)) + "dBm" +
               (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? ",开放" : "") +
               ")</option>";
  }
  if (n == -1) {
    options = "<option value=\"\">扫描失败，请手动输入</option>";
  }
  WiFi.scanDelete();
  return options;
}

void NetManager::handleRoot() {
  String page;
  page.reserve(3072);
  page += F("<!DOCTYPE html><html lang='zh-CN'><head><meta charset='utf-8'>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<title>ESP32 天气时钟配网</title><style>"
            "body{font-family:system-ui,sans-serif;max-width:420px;margin:40px auto;"
            "padding:0 16px;line-height:1.8}"
            "label{display:block;margin-top:14px;font-weight:600}"
            "input,select{width:100%;padding:10px;font-size:16px;"
            "border:1px solid #ccc;border-radius:8px;box-sizing:border-box}"
            "button{margin-top:20px;width:100%;padding:12px;font-size:16px;"
            "background:#2f6b52;color:#fff;border:0;border-radius:8px}"
            ".tip{font-size:13px;color:#888;margin-top:10px}"
            "</style></head><body>");
  page += F("<h2>ESP32 天气时钟 · 配网</h2>");
  page += F("<form method='POST' action='/save'>");
  page += F("<label>选择 WiFi</label><select name='ssid'>");
  page += scanNetworksHtml();
  page += F("</select>");
  page += F("<label>SSID（也可手动输入）</label><input name='ssid' list='none'>");
  page += F("<label>密码</label>"
            "<input name='password' type='password' placeholder='开放网络可留空'>");
  page += F("<button type='submit'>保存并连接</button>");
  page += F("</form>");
  page += F("<p class='tip'>连接成功后设备会自动重启。"
            "长按开发板 BOOT 键 5 秒可清除已保存的 WiFi。</p>");
  page += F("</body></html>");

  g_server.send(200, "text/html; charset=utf-8", page);
}

void NetManager::handleSubmit() {
  if (!_inApMode) { g_server.send(404); return; }

  const String ssid = g_server.arg("ssid");
  const String pass = g_server.arg("password");

  if (ssid.length() == 0) {
    g_server.send(400, "text/html; charset=utf-8",
                  "<p>SSID 不能为空，请返回重填。</p>");
    return;
  }

  Serial.printf("[Net] 收到配网请求：%s\n", ssid.c_str());
  g_server.send(200, "text/html; charset=utf-8",
                "<p>正在连接，请稍候…若长时间无响应请重新配网。</p>");

  blinkLed(2, 200);

  // 连一次即可，WiFi 库会自动把凭据写进 NVS
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  const uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (static_cast<uint32_t>(millis() - startMs) > 20000UL) break;
    delay(200);
  }

  if (connected()) {
    Serial.printf("[Net] 配网成功：%s\n", WiFi.SSID().c_str());
    g_server.send(200, "text/html; charset=utf-8",
                  "<p>配置成功，设备即将重启。</p>");
    delay(800);
    ESP.restart();
  } else {
    Serial.println(F("[Net] 配网失败"));
    stopWebServer();
    g_dns.stop();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    g_dns.start(53, WIFI_AP_SSID, WiFi.softAPIP());
    startWebServer();
    g_server.send(200, "text/html; charset=utf-8",
                  "<p>连接失败，请确认密码后重试。</p>");
  }
}

void NetManager::handleNotFound() {
  g_server.send(404, "text/plain; charset=utf-8",
                "404 Not Found\n请打开 http://192.168.4.1/ 进行配网");
}

void NetManager::setLed(bool on) {
#if PIN_LED >= 0
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, on ? LED_ON : LED_OFF);
#else
  (void)on;
#endif
}

void NetManager::blinkLed(int times, int intervalMs) {
#if PIN_LED >= 0
  for (int i = 0; i < times; ++i) {
    digitalWrite(PIN_LED, LED_ON);
    delay(intervalMs);
    digitalWrite(PIN_LED, LED_OFF);
    delay(intervalMs);
  }
#else
  (void)times; (void)intervalMs;
#endif
}

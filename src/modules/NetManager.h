/**
 * NetManager.h —— WiFi 连接与 SoftAP 配网
 *
 * 配网流程（沿用原项目的方案，代码重写）：
 *   1. 开机尝试连已保存的 WiFi（NVS）
 *   2. 超时或失败 → 开一个临时热点 ESP32-Weather-Setup
 *   3. 手机连上热点 → 浏览器打开 http://192.168.4.1 填 SSID / 密码
 *   4. 提交后 ESP32 连一次，成功即写入 NVS 并重启
 *   5. 长按 BOOT 键 5 秒可清除已保存的凭据，重新配网
 */
#pragma once

#include <Arduino.h>
#include "config.h"

class NetManager {
 public:
  /// 启动连接流程：先连已有凭据，失败自动进配网模式。阻塞直到有结果。
  void begin();

  /// 主循环里周期调用：处理配网请求、维持连接
  void loop();

  bool connected() const;

  /// 配网模式下热点网关地址（形如 "192.168.4.1"），
  /// 主循环要显示给用户，不该直接依赖 WiFi 全局对象
  String softApIp() const;

  /// 清除 NVS 中保存的 WiFi 凭据
  static void forget();

 private:
  void startAccessPoint();
  void startWebServer();
  void stopWebServer();

  void handleRoot();
  void handleSubmit();
  void handleNotFound();

  /// 扫描附近热点，生成 <option> 列表
  String scanNetworksHtml();

  void setLed(bool on);
  void blinkLed(int times, int intervalMs);

  bool _inApMode = false;
  bool _connected = false;
  uint32_t _lastReconnectMs = 0;
};

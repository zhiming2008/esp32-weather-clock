#include "Display.h"

#include "../fonts/Deng28.h"   // 自制中文点阵字库（28px）

TFT_eSPI Display::_tft = TFT_eSPI();

namespace {

String pad2(int v) {
  // 注意：这里不能用 F("0")，三元运算符两支类型不一致（FlashStringHelper* vs String）会编译失败
  return (v < 10 ? String("0") : String()) + String(v);
}

String withUnit(const String& value, const char* unit) {
  return value + unit;
}

}  // namespace

void Display::begin() {
  _tft.begin();
  _tft.setRotation(SCREEN_ROTATION);
  _tft.fillScreen(Palette::Background);

#if PIN_TFT_BACKLIGHT >= 0
  pinMode(PIN_TFT_BACKLIGHT, OUTPUT);
  digitalWrite(PIN_TFT_BACKLIGHT, HIGH);
#endif

  // 让中文字模先常驻，避免第一帧还在下载字库时显示乱码
  _tft.loadFont(Deng28);
  _tft.setTextColor(Palette::Text);
  drawFrame();
  _tft.unloadFont();
}

void Display::drawFrame() {
  _tft.drawFastVLine(Layout::DIVIDER_X, 0, _tft.height(), Palette::Text);
  _tft.drawFastHLine(Layout::DIVIDER_X, Layout::HLINE_Y,
                     _tft.width() - Layout::DIVIDER_X, Palette::Text);
}

void Display::drawClock(const DateTimeInfo& t) {
  if (!t.valid) {
    showStatus(F("正在同步时间..."));
    return;
  }

  // ---- 秒：每秒擦一次 ----
  _tft.fillRect(Layout::SEC_BOX_X, Layout::SEC_BOX_Y,
                Layout::SEC_BOX_W, Layout::SEC_BOX_H,
                Palette::Background);
  _tft.setTextColor(Palette::Alert);
  _tft.setTextSize(4);
  _tft.drawString(pad2(t.second), Layout::SEC_X, Layout::SEC_Y);
  _tft.setTextSize(1);

  // ---- 分：整分钟擦一次 ----
  if (t.second == 0) {
    _tft.fillRect(Layout::MIN_BOX_X, Layout::MIN_BOX_Y,
                  Layout::MIN_BOX_W, Layout::MIN_BOX_H,
                  Palette::Background);
  }
  _tft.setTextColor(Palette::Accent);
  _tft.setTextSize(5);
  _tft.drawString(pad2(t.minute), Layout::MIN_X, Layout::MIN_Y);

  // ---- 时 + 冒号 ----
  _tft.drawString(pad2(t.hour), Layout::HOUR_X, Layout::HOUR_Y);
  _tft.drawString(":", Layout::COLON_X, Layout::HOUR_Y);
  _tft.setTextSize(1);

  // ---- 星期：整点擦一次 ----
  if (t.minute == 0 && t.second == 0) {
    _tft.fillRect(Layout::WEEKDAY_BOX_X, Layout::WEEKDAY_BOX_Y,
                  Layout::WEEKDAY_BOX_W, Layout::WEEKDAY_BOX_H,
                  Palette::Background);
  }

  // ---- 月日：只在每天零点擦一次 ----
  const bool midnight = (t.hour == 0 && t.minute == 0 && t.second == 0);
  if (midnight) {
    _tft.fillRect(Layout::DATE_BOX_X, Layout::DATE_BOX_Y,
                  Layout::DATE_BOX_W, Layout::DATE_BOX_H,
                  Palette::Background);
  }

  _tft.setTextColor(Palette::Secondary);
  _tft.setTextSize(4);
  _tft.drawString(pad2(t.month), Layout::MONTH_X, Layout::DATE_Y);
  _tft.drawString(pad2(t.day),   Layout::DAY_X,   Layout::DATE_Y);
  _tft.setTextSize(1);

  // ---- 星期：中文，走自制字库 ----
  static const char* const kWeekday[7] = {
      F("周日"), F("周一"), F("周二"), F("周三"),
      F("周四"), F("周五"), F("周六")};

  _tft.loadFont(Deng28);
  _tft.setTextColor(Palette::Text);
  _tft.drawString(kWeekday[t.weekday % 7], Layout::WEEKDAY_X, Layout::DATE_Y);
  _tft.unloadFont();
}

void Display::drawWeather(const WeatherNow& w) {
  if (!w.valid) {
    _tft.loadFont(Deng28);
    _tft.setTextColor(Palette::Text);
    _tft.drawString(F("天气获取失败"), Layout::WEATHER_X1, Layout::WEATHER_Y1);
    _tft.unloadFont();
    return;
  }

  _tft.loadFont(Deng28);
  _tft.setTextColor(Palette::Text);

  _tft.drawString(F("天气:") + w.text,
                  Layout::WEATHER_X1, Layout::WEATHER_Y1);
  _tft.drawString(withUnit(w.temp, "℃"),
                  Layout::WEATHER_X1, Layout::WEATHER_Y2);
  _tft.drawString(F("体感:") + withUnit(w.feelsLike, "℃"),
                  Layout::FEEL_X, Layout::FEEL_Y);
  _tft.drawString(F("湿度:") + withUnit(w.humidity, "%"),
                  Layout::HUMID_X, Layout::HUMID_Y);
  _tft.drawString(F("风力:") + w.windScale + F("级"),
                  Layout::WIND_X, Layout::WIND_Y);

  _tft.unloadFont();
}

void Display::drawClimate(const ClimateReading& c) {
  // 传感器块每周 2 秒才更新一次，重画前先擦干净
  _tft.fillRect(Layout::SENSOR_BOX_X, Layout::SENSOR_BOX_Y,
                Layout::SENSOR_BOX_W, Layout::SENSOR_BOX_H,
                Palette::Background);

  _tft.loadFont(Deng28);
  _tft.setTextColor(Palette::Text);

  if (c.valid) {
    _tft.drawString(F("温度:") + String(c.temperature, 1) + F("℃"),
                    Layout::SENSOR_BOX_X + 2, 20);
    _tft.drawString(F("湿度:") + String(c.humidity, 1) + F("%"),
                    Layout::SENSOR_BOX_X + 2, 100);
  } else {
    _tft.drawString(F("传感器读取失败"), Layout::SENSOR_BOX_X + 2, 20);
  }

  _tft.unloadFont();
}

void Display::showStatus(const String& line1, const String& line2) {
  _tft.fillScreen(Palette::Background);
  _tft.loadFont(Deng28);
  _tft.setTextColor(Palette::Text);
  _tft.drawString(line1, 20, 140);
  if (line2.length()) {
    _tft.drawString(line2, 20, 190);
  }
  _tft.unloadFont();
  Serial.printf("[Display] %s %s\n", line1.c_str(), line2.c_str());
}

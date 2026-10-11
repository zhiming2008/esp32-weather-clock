/**
 * secrets.example.h —— 密钥配置模板
 *
 * 使用方法：
 *   1. 复制本文件为 include/secrets.h
 *   2. 把下面的占位符换成你自己的真实值
 *   3. include/secrets.h 已被 .gitignore 忽略，不会误传到仓库
 *
 * 获取和风天气 API Key：https://devapi.qweather.com/
 *   免费订阅 → API 订阅 → 选择「免费订阅」→ 拿到 key
 *   Key 是绑定账号的，公开仓库里的 key 会被别人刷配额，
 *   泄露后请立刻到控制台重置。
 */

#pragma once

// 和风天气 API Key —— 替换成你自己的
#define HEFENG_API_KEY "YOUR_HEFENG_API_KEY_HERE"

// 城市 ID（LocationID），获取入口：
//   https://devapi.qweather.com/docs/geo/city-lookup.html
//   也可以直接填经纬度，例如 "116.41,39.92"
#define CITY_ID "101270401"   // 绵阳，如需换城市请改这里

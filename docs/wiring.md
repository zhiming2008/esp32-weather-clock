# 接线表

> 全部引脚定义在 [`include/config.h`](../include/config.h) 与
> [`include/User_Setup.h`](../include/User_Setup.h) 两处，改硬件只动这两个文件。

## 硬件清单

| 器件 | 型号 / 规格 | 备注 |
|---|---|---|
| 主控 | ESP32-DevKitC（ESP32-32D，30 脚） | 双核 240MHz，4MB Flash |
| 显示 | 3.5 寸 SPI 串口 TFT，320×480，ILI9488 驱动 | 带电阻触摸，**本项目未启用触摸** |
| 传感器 | DHT11 温湿度模块 | 单总线，采样间隔必须 ≥ 2s |
| 其它 | 无 SD 卡 | |

## 引脚对照

### TFT 屏（SPI 接口）

| TFT 丝印 | ESP32 GPIO | 说明 |
|---|---|---|
| VCC | 3V3 | |
| GND | GND | |
| DIN / MOSI | **GPIO23** | 数据 |
| CLK / SCLK | **GPIO18** | 时钟 |
| DO / MISO | **GPIO19** | 数据回读（屏的 SDO） |
| CS | **GPIO5** | 片选 |
| DC / RS | **GPIO2** | 数据/命令 |
| RST | **GPIO4** | 复位 |
| BL / LED | 3V3 | 背光常亮，未接 GPIO |
| SDI / SDO / CS | 悬空 | 本项目不使用 SD 卡 |

### DHT11

| DHT 丝印 | ESP32 GPIO |
|---|---|
| VCC | 3V3 |
| DATA | **GPIO17** |
| GND | GND |

> 建议在 DATA 与 3V3 之间并一个 4.7kΩ～10kΩ 上拉电阻。
> 模块上一般自带上拉，没有的话必须加，否则读数会随机跳变。

### BOOT 键

| 丝印 | ESP32 GPIO | 功能 |
|---|---|---|
| IO0 / BOOT | **GPIO0** | **长按 5 秒**清除已保存的 WiFi 凭据并进入配网模式 |

### 状态 LED

| 位置 | 说明 |
|---|---|
| 板载 RGB LED | GPIO2 —— ⚠️ **与 TFT_DC 冲突** |

> ESP32-DevKitC 的板载 RGB LED 数据脚是 GPIO2，而本项目的 TFT_DC 也用 GPIO2。
> 两者接在一起可能互相干扰。如果屏幕显示异常，把 `config.h` 里的
> `PIN_LED` 改成任意空闲 GPIO 并外接一个 LED，程序会自动改用新引脚。
> 或者干脆设 `PIN_LED` 为 `-1` 关闭 LED 功能（代码已做 `#if` 保护）。

## 已知的硬件注意点

1. **SDO 接了 MISO 之后，同一条 SPI 总线上不能再挂会回灌电平的器件。**
   本项目总线上只有屏和触摸，所以没问题；若以后加 SD 卡或 SPI 器件，
   需要断开屏的 MISO，或给 MISO 加一个三态缓冲。

2. **ILI9488 初始化失败时先怀疑 SPI 接线**，尤其是 GND 没共地、
   杜邦线太长导致信号衰减。SPI 频率可从 20MHz 降到 8MHz 试。

3. **DHT11 别贴着 ESP32 和 TFT 放**，后两者发热会直接影响温湿度读数。

## 首次上电步骤

1. 按上表接好线，注意 GND 必须共地
2. 烧录固件
3. 设备开一个名为 `ESP32-Weather-Setup` 的热点
4. 手机连该热点 → 浏览器打开 `http://192.168.4.1`
5. 填写家里 WiFi → 设备连接成功后自动重启
6. 之后就会显示 时间 / 日期 / 星期 / 天气 / 温湿度

重新配网：**长按 BOOT 键 5 秒**。

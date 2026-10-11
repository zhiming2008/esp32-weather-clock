/**
 * User_Setup.h —— TFT_eSPI 显示与触摸驱动配置
 *
 * 这个文件原本躺在 .pio/libdeps/ 里（属于第三方库的私有目录，
 * PlatformIO 每次重新拉依赖就可能被覆盖）。现在移到仓库的 include/ 下，
 * 由 platformio.ini 的 build_flags 强制注入：
 *
 *     -D USER_SETUP_LOADED=1
 *     -include include/User_Setup.h
 *
 * 硬件：3.5 寸 SPI 串口 TFT，320×480，ILI9488 驱动芯片，带触摸
 *
 * 注意：本文件按 TFT_eSPI 的约定**故意不加 include guard**，
 * 因为它是通过编译器的 -include 预包含的，本身就是第一个被解析的头文件。
 */

/* ==================== 面板驱动 ==================== */
#define ILI9488_DRIVER
// ⚠️ ILI9488 的 SDO 接到 MISO 时，若同一条 SPI 总线上还挂了别的器件，
//    且那些器件在 CS 拉高后不回三角，读取会出错。本项目屏上未接 SD 卡，
//    总线上只有屏和触摸，保留 MISO 没问题。

/* ==================== 接线引脚 ==================== */
#define TFT_MISO  19
#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_CS     5
#define TFT_DC     2
#define TFT_RST    4

// TFT_BL 保持 -1：背光不接 GPIO，模块自带限流电路，常亮。
#define TFT_BL    -1

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// 触摸（XPT2046 电阻式）。当前程序只做显示，暂未启用触摸，
// 所以不定义 TFT_TOUCH_CS / IRQ。需要时取消下面两行的注释并填实际引脚。
// #define TFT_TOUCH_CS   33
// #define TFT_TOUCH_IRQ  32

/* ==================== SPI 频率 ==================== */
// 20MHz：ILI9488 官方推荐上限附近，配短线足够刷 320×480。
// 若屏离开发板较远或有噪点，降到 8MHz 试试。
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY   2500000

/* ==================== 字体 ==================== */
#define LOAD_GLCD     // 字库 1：Adafruit 8 像素原始字体，FLASH 占用约 1820 B
#define LOAD_FONT2    // 字库 2：16 像素小字，FLASH 约 3534 B，96 字符
#define LOAD_FONT4    // 字库 4：26 像素中字，FLASH 约 5848 B，96 字符
#define LOAD_FONT6    // 字库 6：48 像素大数字，仅 1234567890:-.apm
#define LOAD_FONT7    // 字库 7：七段 48 像素，仅 1234567890:-
#define LOAD_FONT8    // 字库 8：75 像素超大数字，仅 1234567890:-
#define LOAD_GFXFF    // FreeFonts：48 种 Adafruit_GFX 可缩放字体
#define SMOOTH_FONT   // 抗锯齿渲染（配合 FreeFonts 使用）

/* 中文点阵字库说明：
 * src/fonts/Deng28.h 由 TFT_eSPI 官方工具
 *   "Tools → Smooth Fonts → Make Font Generator"
 * 生成，字号选 28 px、DMA 可关，字符集至少包含：
 *   0-9  ℃ % 周日一二三四五六 天气温度体感相对湿度风力等级无网络 请稍候等待连接
 * 换字号/换字集时重新生成替换即可，程序里用 tft.loadFont(Deng28) 加载。
 */

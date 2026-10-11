# 中文点阵字库

`Deng28.h` 是 **28 像素中文点阵字模**，由 TFT_eSPI 官方工具生成，
是一个 2.5MB / 27072 行的十六进制数组（`PROGMEM`，存在 Flash 里不占 RAM）。

## 怎么生成的

TFT_eSPI 自带的离线字库生成器：

1. PlatformIO → `pio run -t build` 至少成功一次（生成工具依赖编译产物）
2. 打开 VS Code 的 PlatformIO 面板 → **Tools → Smooth Fonts → Make Font Generator**
3. 常用工具路径：`<项目>/.pio/libdeps/esp32dev/TFT_eSPI/Tools/`
4. 在浏览器里打开 `Create_Smooth_Font.html`，参数：

| 项 | 值 |
|---|---|
| Font size (height) | `28` |
| Character range | 见下 |
| Anti-alias | 开启 |
| Run-length encoding | 开启（可减小文件） |
| DMA mode | 关闭 |

## 字符集

当前字库里至少要包含下面这些字符，缺一个显示出来就是空白：

```
数字 0-9  ℃ % : -
星期 周日一二三四五六
天气  天 气 预 报
温度  温 度 体 感
湿度  相 对 湿 度
风力  风 力 等 级
状态  正 在 同 步 时 间 获 取 失 败
      配 置 WiFi 请 等 待 候 连 接
      传 感 器 读 取
```

用工具的 **"Save as array"** 导出后，把文件命名为 `Deng28.h`
放在本目录即可。程序里通过 `tft.loadFont(Deng28)` 加载。

## 为什么还要留在仓库里

字库有 2.5MB，直接生成需要跑一遍工具，比较麻烦；
而且换字集后重新生成会产生巨大的 diff。所以选择保留在仓库里，
换字号/换字集时再按上面的流程覆盖。

> 如果不介意 clone 之后自己生成一步，可以在 `.gitignore` 里加上
> `src/fonts/Deng28.h`，并把 `main.cpp` 中加载字库的那几处改成条件编译。

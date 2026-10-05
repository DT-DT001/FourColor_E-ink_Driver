# FourColor E-ink Driver

华为手机壳拆机四色墨水屏 **MSPQ1TMN1001QNX** 的可移植 C 驱动，包含 ESP-IDF 适配、其他 MCU 的 HAL 模板和离线抖动转换工具。

- 屏幕型号：`MSPQ1TMN1001QNX`；本项目样机排线：`SE0398NZ07-FPC-AX`。
- 逻辑画面：**768×552**；控制器扫描范围：768×600。
- 原生颜色：黑、白、黄、红；每像素 2 bit，每帧 **105984 字节**。
- 默认 A1 行映射，保留 A0。AX 排线样机使用 A1 完成刷新，但其他拆机批次仍需核对行序。
- 核心不依赖操作系统，不分配全屏缓冲，通过五个 HAL 回调移植。

本仓库只保留驱动、文档、可编译的测试图示例与图片转换工具。**不包含插画合集、图片数组、预编译固件、构建目录、SDK、日志或移植包 ZIP。**

## 接线

| 信号 | ESP32-P4 示例 GPIO | 说明 |
| --- | --- | --- |
| DIN | IO2 | 数据输出 |
| CLK | IO3 | 时钟输出，空闲低 |
| CS | IO4 | 片选，低有效 |
| RST | IO5 | 复位，低有效 |
| DC | IO6 | 低=命令，高=数据 |
| BUSY | IO7 | 输入，**低=忙，高=就绪** |

沿用屏幕已能工作的转接/供电电路，主控与屏幕共地。ESP32-P4 GPIO 为 3.3V 逻辑，不要直接接 5V 信号；本仓库不包含拆机裸屏的电源电路设计。

## ESP-IDF 快速验证

样机环境：ESP-IDF **5.5.3**，ESP32-P4 **rev 3.2 / 16MB Flash**。示例默认最低芯片版本为 3.1；其他开发板应先调整芯片版本、目标和 Flash 容量。

在已激活 ESP-IDF 的命令行执行：

```sh
git clone https://github.com/DT-DT001/FourColor_E-ink_Driver.git
cd FourColor_E-ink_Driver/examples/espidf
idf.py set-target esp32p4
idf.py build
idf.py -p COM23 flash monitor
```

替换自己的串口。示例在 RAM 中生成黑/白/黄/红色条和黑白点阵，成功后只刷新一次并休眠保留画面，不反复刷屏；失败时每 60 秒重试。此示例需约 104KiB 图像 RAM，真实图片可改为从 Flash 中的 `const` 数据读取。

已有 ESP-IDF 工程可直接复制 `components/mspq1tmn1001qnx_epd` 到工程的 `components` 目录。

## 移植与数据格式

详见 [移植说明](docs/PORTING.md)，包括初始化、休眠、超时、其他 MCU 模板与图片格式。

最小移植仅需 `mspq1tmn1001qnx.c`、其头文件及你的 HAL 实现；非 ESP-IDF 平台不编译 `_espidf.c`。普通 CMake 项目也可 `add_subdirectory()` 本仓库并链接 `FourColorEink::Driver`。

屏幕输入为行优先 2bpp：黑=0、白=1、黄=2、红=3；每字节四像素，最高位像素在前。驱动已处理物理方向和扫描行映射，不要重复翻转。

## 离线图片转换与抖动

```sh
python -m pip install -r tools/requirements.txt
python tools/convert_image.py input.png output.bin --preview preview.png --c-array image.c --symbol epd_image
```

工具居中裁剪/缩放到 768×552，默认使用蛇形 Floyd–Steinberg 抖动，可选 `--dither atkinson` 或 `--dither none`。透明区域合成到白底；预览从已打包数据解码得到。输出文件为本地生成，不需要提交到仓库。

灰度观感来自黑白点阵的空间混合，不是原生灰阶驱动模式。转换工具限制中性色只使用黑白，红/黄各自保留色相，避免人脸出现错误彩色噪点；单片机无需运行抖动算法。

标准库单元测试（不需要安装 Pillow）：

```sh
python -m unittest discover -s tests -v
```

完整刷新在原样机约 19 秒，刷新完成后才能计算图片停留时间。驱动默认刷新超时为 180 秒；不要为了“10 秒停留”将超时设成 10 秒。

参考来源与说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

# 使用与移植说明

## 目录与最小依赖

```text
components/mspq1tmn1001qnx_epd/
  mspq1tmn1001qnx.c             平台无关核心
  mspq1tmn1001qnx_espidf.c       ESP-IDF GPIO/RTOS 适配
  include/*.h                  接口
examples/espidf/                独立 ESP32-P4 测试工程
examples/portable/              其他平台适配模板
examples/common/                由代码生成的测试图，不含图片资源
tools/                         离线转换与抖动
tests/                         图片编码与抖动的单元测试
```

原驱动文件保留不变。项目没有捆绑插画数据；生成真实图片见根目录 README 的转换命令。

## 实现 HAL

其他主控加入核心 `mspq1tmn1001qnx.c` 和 `include` 路径，不要编译 `_espidf.c`。使用 C99 或更新标准。

参考 `examples/portable/board_epd_port.c` 填完 TODO。模板未实现时主动返回错误，不是虚拟硬件驱动。

| 回调 | 要求 |
| --- | --- |
| `write(context, signal, level)` | 映射 DIN/CLK/CS/RST/DC，返回**操作是否成功**；写 LOW 成功也返回 true |
| `read_busy(context, &level)` | 写入 BUSY **原始电平**，高=就绪，低=忙；返回读操作是否成功 |
| `delay_us(context, us)` | 微秒延时，至少等待要求的时长；初始时钟半周期 1µs |
| `delay_ms(context, ms)` | 毫秒延时，RTOS 下应让出 CPU；正延时不能截断到 0 tick |
| `millis(context)` | 持续递增的 `uint32_t` 毫秒计时，等待期间也必须递增 |

`context` 可指向板级状态对象；该对象必须在刷新期间有效。勿将已离开作用域的局部对象作为 context。GPIO 初始 CS/RST 高、CLK/DC/DIN 低，BUSY 输入；原 ESP-IDF 适配给 BUSY 配置上拉。

驱动通过 GPIO 位操作发送 MSB-first 数据，CLK 空闲低；不必使用硬件 SPI。HAL 当前不是 SPI 字节传输接口，需要硬件 SPI 时自行适配发送层并验证时序。

## 显示一张图片

转换工具生成 `const uint8_t epd_image[105984]`。将该 C 文件加入构建，并保证数组由链接器放到可普通指针读取的 Flash/只读区。

```c
#include "board_epd_port.h"

extern const uint8_t epd_image[MSPQ_EPD_FRAME_BYTES];

mspq_status_t show_image(void)
{
    if (!board_epd_port_init()) {
        return MSPQ_ERR_HAL;
    }
    const mspq_epd_config_t cfg = {
        .revision = MSPQ_REVISION_A1,
        .ready_timeout_ms = 10000,
        .refresh_timeout_ms = 180000,
        .sleep_after_refresh = true,
    };
    mspq_epd_t panel;
    mspq_status_t status = mspq_epd_init(&panel, board_epd_port_hal(), &cfg);
    if (status == MSPQ_OK) {
        status = mspq_epd_display(&panel, epd_image, sizeof(epd_image));
    }
    return status;
}
```

无图片时可调用 `portable_example_show_test_pattern()`；该示例要同时加入 `examples/common/test_pattern.c` 与相应头文件路径，并需要 105984 字节静态 RAM。

核心自身不分配整屏缓冲，图片指针在同步 `display()` 返回前必须有效。Flash 内存映射数据可以直接读；文件系统、外置非内存映射存储需要先载入连续缓冲，或另行扩展数据读取接口。需要 `PROGMEM` 等特殊读取方式的平台不能直接沿用普通指针访问。

## 2bpp 格式

逻辑分辨率 768×552，每行 192 字节，每帧 105984 字节。图像从左上角逐行、从左到右存储，每字节最高两位是第一个像素。

| 值 | 黑 | 白 | 黄 | 红 |
| --- | --- | --- | --- | --- |
| 2 位编码 | `00` | `01` | `10` | `11` |
| 数值 | 0 | 1 | 2 | 3 |

黑/白/黄/红四像素为 `0x1B`；全黑、全白、全黄、全红分别填充 `0x00`、`0x55`、`0xAA`、`0xFF`。不要把 768×600 的控制器扫描范围当成图像尺寸，也不要把包内格式额外镜像或翻转。

## 刷新状态与轮播

推荐顺序：每张图 `init()` → `display()` → 成功后停留 → 下一张重新 `init()`。

- `display()` 同步等待 BUSY，并在成功刷新后自动 power-off；默认配置还会进入深度休眠。
- 休眠后 `initialized` 为 false，必须复位并重新初始化。
- 即使关闭 `sleep_after_refresh`，刷新后仍有 power-off，不要假设可以跳过下一张的重新上电/初始化。
- 不要在自动休眠后重复调用 `power_off()` / `sleep()`。
- 若轮播要求每张停留 10 秒，在成功返回后延时 10000ms；刷新本身另需时间。
- 错误时不推进图片索引；记录错误，延时再重试，避免紧循环。

同一总线只由一个任务操作，或添加互斥。不要在中断中刷新整屏，不要长时间关闭中断；保留驱动每 8 行的毫秒让步，避免 RTOS 看门狗和计时停止。

## 扫描映射与排错

默认 `MSPQ_REVISION_A1` 沿用当前 AX 排线样机配置，A0 屏可改为 `MSPQ_REVISION_A0`。不同批次需确认行序，不能仅凭 AX 后缀判断所有屏幕。

可用 `mspq_epd_status_name()` 解释状态：

- `MSPQ_ERR_ARGUMENT`：参数或初始化/休眠状态不正确。
- `MSPQ_ERR_HAL`：GPIO 回调失败，或模板尚未完成。
- `MSPQ_ERR_BUSY_TIMEOUT`：检查 BUSY/RST/供电/共地，以及毫秒计时是否继续运行；不要强制返回“就绪”绕过检查。

画面错行优先核对 A0/A1 配置和尺寸；颜色错误核对 2bpp 编码与 DC；看门狗问题核对 RTOS 延时是否截成零。

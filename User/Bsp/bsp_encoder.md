# bsp_encoder

> 代码：`bsp_encoder.hpp` / `bsp_encoder.cpp`

一个实例 = 一个定时器的**编码器接口**（CubeMX 配成 `Encoder TI12`，A/B 两相 4 倍频）。
本类只做两件事：把 16 位 `CNT` 换算成「增量」与「累计位置」，并处理 0 / 65535 回绕。
PSC、ARR、输入滤波、极性全部沿用 `MX_TIMx_Init()`，驱动一个寄存器都不改。

## 接口

| 接口 | 说明 |
| --- | --- |
| `init()` | 启动计数（`HAL_TIM_Encoder_Start`，幂等）；以当前 CNT 为基准把累计位置清零 |
| `read_delta()` | 上次调用以来的增量，**并累加进** `position()` |
| `position()` | 上电（或上次 `reset()`）以来的累计位置，`int32`，可正可负 |
| `read_raw()` | 计数器原始值（`int16`，已按 `invert` 取反） |
| `reset()` | CNT、增量基准、累计位置一起清零（不停止计数） |

## 用法

```cpp
// 10 ms 周期任务里取速度
const int32_t delta = bsp_encoder1.read_delta();          // 本周期计数
const float   rpm   = delta * 6000.0f / COUNTS_PER_REV;   // 计数/10ms → 转/分
```

换算常数：**每圈计数 = 编码器线数 × 4**（TI12 模式对 A、B 两相的上升沿都计数）。
减速比、轮径这类机构参数留在 Device / Module 层，不要写进 BSP。

## 注意

- **调用周期决定了能测多快**：增量靠 `(int16_t)(now - last)` 算，两次调用之间走过
  超过 32767 个计数就会判错方向。10 ms 周期下可测 3.27 M 计数/秒，远大于一般编码器。
- 16 位计数器回绕（65535 → 0 或 0 → 65535）不需要额外处理，上面的补码减法天然正确。
- 方向反了就把 `Config::invert` 改成 `true`，不要改接线，也不要改 CubeMX 的极性。
- 每个实例的 `read_delta()` / `reset()` 只能由**同一个任务**调用（非线程安全）。
- 计数含机械抖动：需要平稳速度时空闲时做一阶滤波，BSP 不做滤波。
- 定时器被编码器占用后**不能再当 PWM 用**（同一 TIM 的 CH1/CH2 已作输入捕获），
  本板 TIM1 / TIM4 / TIM8 已经归编码器。

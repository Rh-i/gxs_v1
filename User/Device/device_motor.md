# device_motor

> 代码：`device_motor.hpp` / `device_motor.cpp`

一个实例 = 一个直流电机：**两路 PWM（H 桥 IN1/IN2）+ 一个编码器 + 一套速度环 PID**。
PWM 走 `BspPwm`、计数走 `BspEncoder`、算法走 `Algorithm/Pid`，本类只做器件语义与换算。

## 绑定

```cpp
DeviceMotor motor1({
  .pwm_a = &bsp_pwm3,     // 任意一路 BspPwm
  .pwm_b = &bsp_pwm4,     // 任意另一路 BspPwm（不能与 pwm_a 同一个通道）
  .encoder = &bsp_encoder2, // 任意一个 BspEncoder（不要求与 PWM 同序号）
  .encoder_lines = 500.0f,
  .encoder_multiplier = 4.0f,
  .gear_ratio = 32.0f,
  .wheel_diameter_cm = 7.0f,
});
```

- `pwm_a` / `pwm_b` / `encoder` **互不相关**，按实际接线自由组合。
- 三路指针没填齐时 `device_init()` 跳过该电机、`motor_task` 也不会碰它 —— 上电不会因此停机。

## 换算

```
输出轴每转计数 = 编码器线数 × 倍频 × 减速比
每转行程 (cm)  = π × 轮子直径 (cm)
counts_per_cm  = 输出轴每转计数 / 每转行程
```

本机构（500 线、TI12 4 倍频、1:32、轮直径 7 cm）：

| 量 | 值 |
| --- | --- |
| 输出轴每转计数 | 500 × 4 × 32 = **64000** |
| 每转行程 | π × 7 = **21.99 cm** |
| `counts_per_cm` | ≈ **2910.3 计数/cm** |
| 1 cm/s 对应 | ≈ 2910 计数/秒 |
| 测速量化（1 ms 一拍） | 1 计数 ≈ **0.34 cm/s** |

量化误差随周期变长而变小：2 ms 一拍约 0.17 cm/s。低速段想要更平滑就在上层做滤波。

## 一拍里做什么（`update(dt_s)`）

1. `delta = encoder->read_delta()`（16 位回绕已处理）
2. `speed = delta / counts_per_cm / dt_s` → cm/s
3. 速度+时间模式：`remaining_s -= dt_s`，到点 `stop()` 并返回 `true`
4. **零速死区**：`|target|` 与 `|speed|` 同时小于 `deadband_cm_s` → 输出 0、清一次积分、本拍返回
5. `u = pid.calc(target_cm_s, speed_cm_s)`，输出量纲 = 占空比 %
6. 限幅 `±out_max` → 按 `invert` 取反 → **双 PWM 互斥输出**

停止模式不跑 PID，但**照样读增量**，所以手转电机时 `speed_cm_s()` 一直有数，方便验证接线。

## 零速死区

静止时编码器在边沿附近 ±1 抖动，1 ms 一拍就是 ±0.34 cm/s；`kp = 0.5` 下变成 ±0.17% 占空比，
表现为线圈电流的持续抖动（PWM 已提到 20 kHz，听不到啸叫，但电流纹波与发热仍在）。
所以 `|target|` 和 `|speed|` 同时小于 `deadband_cm_s`（默认 **0.4**）时：

- 直接 `_apply_output(0)`，两路 PWM 都 0（滑行，不是刹车）
- 只在**刚进入**死区时 `Pid::reset()`（每拍都清会把微分历史也清掉）
- 模式仍是 SPEED，目标一变大就无缝恢复闭环

代价：小于 0.4 cm/s 的目标不输出。填 0 可关闭死区。

## 双 PWM 互斥（关键）

运行时两路必须互斥：裸半桥同时导通就是直通，会炸管。`_apply_output()` 的顺序固定：

| 目标 | 先写 | 后写 |
| --- | --- | --- |
| `u > 0` | `pwm_b = 0` | `pwm_a = u` |
| `u < 0` | `pwm_a = 0` | `pwm_b = -u` |
| `u == 0`（滑行） | 两路都 0 | — |
| 刹车（`stop()` / 模式 STOP） | 两路都 100% | — |

刹车不是「两路 0」：两路都给满占空比时 H 桥上下管构成回路，电机绕组被短接，动能变成制动电流；
两路 0 则是断开，轮子会惰行。高速时突然刹车电流较大，确认驱动板能受。

## 控制频率与 PID 的关系

`dt_s` 由调用方传入，所以频率是可调的（`motor_task.cpp` 里的 `MOTOR_TASK_PERIOD_MS`）。
但 `Pid` 的 `ki` / `kd` 是**每拍量纲**：

- 频率翻倍 → 同样的 ki，实际积分速度翻倍（等效 Ki 翻倍），kd 同理
- **改频率后必须重新标定**，或者把 ki / kd 按频率比例缩放

`motor_task` 用 `bsp_dwt.get_delta_s()` 实测 dt，任务被抢占时 PID 仍然对得上真实时间。

## 调参起步

1. `ki = kd = 0`，只给 `kp`，从「能跟上但会振荡」的值往回收 50%
2. 加 `ki` 消静差，配合 `Pid::Limit::i_max` 防积分饱和
3. `kd` 一般只在有明显超调时才加（测速本身有量化噪声，D 会把噪声放大）

## 波形（JustFloat）

`Pid` 只负责**组帧**，发不发、什么时候发由你决定；实际发送在 `DeviceMotor::pid_print()` 里：

```cpp
DeviceMotor motor1({ ..., .uart = &bsp_uart3 });   // 绑打印串口

// 你自己的任务里，想发的时候调：
motor1.pid_print();      // 目标值 + 实测值 + 原始输出 → JustFloat 一帧（16 B）
```

- `Pid::justfloat(uint8_t *buf)` 只把 3 个 float 写进调用方的缓冲（返回 16），**不碰串口**
- 通道顺序：`目标值` / `实测值` / `PID 原始输出（占空比 %，未经 invert 取反与 DeviceMotor 再限幅）`
- VOFA+ 选 **JustFloat** 协议，两个通道自动出曲线
- 不阻塞：串口队列满就丢这一帧
- 带宽：1 kHz 单台 = 12 kB/s，USART3 的 115200 只有 11.5 kB/s（三台更不够）——按需降频打印或提速到 921600

## 注意

- `init()` 要求编码器已经 `init()` 过（`bsp_init()` 里做的），否则返回 `NOT_INIT`。
- 两路 PWM 必须是**不同通道**；同一个 `BspPwm` 实例传给 `pwm_a` 和 `pwm_b` 会退化成正反转同占空比。
- 非线程安全：`set_*()` 只写目标值，`update()` 是唯一的 PWM 写入者，必须在同一个任务里。
- 不做刹车/能耗制动：`stop()` 只是把两路 PWM 归 0（滑行）。

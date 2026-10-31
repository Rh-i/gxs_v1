#include "device_cfg.hpp"

#include "bsp_cfg.hpp" // IWYU pragma: keep（bsp_pwm1/2、bsp_uart3）
#include "FreeRTOS.h"  // IWYU pragma: keep (configASSERT)
#include "status.hpp"  // IWYU pragma: keep（device_init 里的 Status）


// ---------------- 全局实例 ----------------

// R9DS 遥控接收机：走 UART4（SBUS，100000/9E2 + 外部反相），由 bsp_uart4 DMA 接收。
// 四个摇杆的零飘偏移用默认全 0：死区已经把"没动杆但不在 1000"处理掉了。
DeviceR9dsT<decltype(bsp_uart4)> r9ds(bsp_uart4);

// 舵机：BSP 层 TIM2 两路 PWM 已配成 50 Hz / 20 ms、1 计数 = 1 µs，脉宽可直接按 µs 填。
// Config 参数顺序 = {通道, 脉宽下限, 脉宽上限, 行程, 限速, 加速度, 反向, 微调}。
// ⚠ 脉宽端点与限速/加速度是**占位值，等实机标定**；限速写 0 = 不限速。
// ⚠ 必须由同一个任务周期调用 servoN.update()（唯一的脉宽写入者），详见 device_servo.md。
DeviceServo servo1(
  {.pwm            = &bsp_pwm1,
   .pulse_min_us   = 500.0f,
   .pulse_max_us   = 2500.0f,
   .range_deg      = 270.0f,
   .max_speed_dps  = 0.0f,
   .max_accel_dps2 = 0.0f});
DeviceServo servo2(
  {.pwm            = &bsp_pwm2,
   .pulse_min_us   = 500.0f,
   .pulse_max_us   = 2500.0f,
   .range_deg      = 180.0f,
   .max_speed_dps  = 300.0f,
   .max_accel_dps2 = 1500.0f});


// 电机：双 PWM（H 桥 IN1/IN2）+ 编码器 + 单套速度环。
// 正方向约定：逆/顺时针以「从外向内看轮子」为准，顺时针为正（方向反了就改该电机的 invert）。
// 机械参数：500 线编码器 × TI12(4 倍频) × 1:32 减速，轮直径 7 cm（半径 35 mm）。
// PID 初值全 0（不输出），上机后再标定 kp / ki / kd。
// 波形：给某台的 Config 里加 `.uart = &bsp_uart3`，再在你自己的任务里调 motorN.pid_print()，
//       就会把「目标值 + 实测值」以 JustFloat（VOFA+）发出去；什么时候发完全由你控制。
//       一帧 16 B（目标/实测/output 三路 float），频率自己控。
// 绑定（pwm_a = 第一路，pwm_b = 第二路）：
//   电机1：TIM3_CH1(PB4) / TIM3_CH2(PB5)   + 编码器1(TIM1  PA8/PA9)
//   电机2：TIM16_CH1(PB8) / TIM17_CH1(PB9) + 编码器2(TIM8  PC6/PC7)
//   电机3：TIM12_CH2(PB15) / TIM12_CH1(PB14) + 编码器3(TIM4 PD12/PD13)
DeviceMotor motor1(
  {.pwm_a              = &bsp_pwm3,
   .pwm_b              = &bsp_pwm4,
   .encoder            = &bsp_encoder1,
   .encoder_lines      = 500.0f,
   .encoder_multiplier = 4.0f,
   .gear_ratio         = 32.0f,
   .wheel_diameter_cm  = 7.0f,
   .invert             = true, // pwm_a 实际是负方向：PID 正输出改走 pwm_b（不动接线）
   .pid                = {{18.0f, 0.016f, 0.0f}, {1000.0f}},
   .uart               = &bsp_uart3});

DeviceMotor motor2(
  {.pwm_a              = &bsp_pwm7,
   .pwm_b              = &bsp_pwm8,
   .encoder            = &bsp_encoder2,
   .encoder_lines      = 500.0f,
   .encoder_multiplier = 4.0f,
   .gear_ratio         = 32.0f,
   .wheel_diameter_cm  = 7.0f,
   .invert             = true, // pwm_a 实际是负方向：PID 正输出改走 pwm_b（不动接线）
   .pid                = {{15.0f, 0.017f, 0.0f}, {1000.0f}},
   .uart               = &bsp_uart3});

DeviceMotor motor3(
  {.pwm_a              = &bsp_pwm5,
   .pwm_b              = &bsp_pwm6,
   .encoder            = &bsp_encoder3,
   .encoder_lines      = 500.0f,
   .encoder_multiplier = 4.0f,
   .gear_ratio         = 32.0f,
   .wheel_diameter_cm  = 7.0f,
   .pid                = {{20.0f, 0.023f, 0.0f}, {1000.0f}},
   .uart               = &bsp_uart3});


// ----------------
// ---------------- 函数定义 ----------------


/** @brief 逐个初始化本层设备：失败就地停机 */
void device_init(void)
{
  configASSERT(r9ds.init() == Status::OK);

  // 舵机：init() 只校验配置、不写脉宽，所以这一步不会让舵机动起来
  configASSERT(servo1.init() == Status::OK); // PB10 TIM2_CH3
  configASSERT(servo2.init() == Status::OK); // PB11 TIM2_CH4

  // 电机：绑定填齐了才初始化（还没接线时跳过，不影响其它外设上电）
  if (motor1.is_bound())
    configASSERT(motor1.init() == Status::OK);
  if (motor2.is_bound())
    configASSERT(motor2.init() == Status::OK);
  if (motor3.is_bound())
    configASSERT(motor3.init() == Status::OK);
}


// ----------------

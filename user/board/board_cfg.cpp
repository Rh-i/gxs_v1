/**
 * @file board_cfg.cpp
 * @brief `board_init()`：把 board_cfg.hpp 里的实例按正确顺序逐个 init()
 *
 * @details 顺序：计时基准 → 外设（UART / PWM / 编码器 / GPIO / 按键）→ 设备。
 *          每一路 init() 都用 ICBSP_CHECK：失败就地停机，fault_record 会记下代码行号。
 */

#include "board_cfg.hpp"

#include "fault_record.hpp" // ICBSP_CHECK

#include "FreeRTOS.h" // IWYU pragma: keep (configASSERT)


void board_init(void)
{
  // ① 计时基准 bsp_dwt 已由 system_init() 内部 init()，这里不再重复初始化

  // ② 串口：DMA 收发（实例在 .dma_buffer）
  ICBSP_CHECK(bsp_uart2.init() == Status::OK);  // PA2/PA3
  ICBSP_CHECK(bsp_uart3.init() == Status::OK);  // 调试打印
  ICBSP_CHECK(bsp_uart4.init() == Status::OK);  // SBUS 接收机（只收）
  ICBSP_CHECK(bsp_uart7.init() == Status::OK);  // PE7/PE8
  ICBSP_CHECK(bsp_uart10.init() == Status::OK); // PE2/PE3

  // ③ PWM：舵机 50 Hz 两路 + 电机 H 桥六路（20 kHz）
  ICBSP_CHECK(bsp_pwm1.init() == Status::OK); // PB10 TIM2_CH3
  ICBSP_CHECK(bsp_pwm2.init() == Status::OK); // PB11 TIM2_CH4
  ICBSP_CHECK(bsp_pwm3.init() == Status::OK); // PB4  TIM3_CH1
  ICBSP_CHECK(bsp_pwm4.init() == Status::OK); // PB5  TIM3_CH2
  ICBSP_CHECK(bsp_pwm5.init() == Status::OK); // PB14 TIM12_CH1
  ICBSP_CHECK(bsp_pwm6.init() == Status::OK); // PB15 TIM12_CH2
  ICBSP_CHECK(bsp_pwm7.init() == Status::OK); // PB8  TIM16_CH1
  ICBSP_CHECK(bsp_pwm8.init() == Status::OK); // PB9  TIM17_CH1

  // ④ 编码器：启动计数（TI12 四倍频）
  ICBSP_CHECK(bsp_encoder1.init() == Status::OK); // TIM1
  ICBSP_CHECK(bsp_encoder2.init() == Status::OK); // TIM8
  ICBSP_CHECK(bsp_encoder3.init() == Status::OK); // TIM4

  // ⑤ 按键：BspGpio 只是引脚封装（引脚方向由 MX_GPIO_Init() 配好），不需要 init()
  ICBSP_CHECK(key_user.init() == Status::OK);

  // ⑥ 设备：必须在 BSP 之后（依赖已 init 的串口 / PWM / 编码器）
  ICBSP_CHECK(r9ds.init() == Status::OK);   // UART4
  ICBSP_CHECK(wt102.init() == Status::OK);  // UART2（230400 8N1）
  ICBSP_CHECK(servo1.init() == Status::OK); // PB10 TIM2_CH3
  ICBSP_CHECK(servo2.init() == Status::OK); // PB11 TIM2_CH4

  // 电机：三路绑定填齐了才初始化（没接线时跳过，不影响其它外设上电）
  if (motor1.is_bound())
  {
    ICBSP_CHECK(motor1.init() == Status::OK);
  }
  if (motor2.is_bound())
  {
    ICBSP_CHECK(motor2.init() == Status::OK);
  }
  if (motor3.is_bound())
  {
    ICBSP_CHECK(motor3.init() == Status::OK);
  }
}

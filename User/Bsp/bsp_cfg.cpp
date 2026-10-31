#include "bsp_cfg.hpp"

#include "FreeRTOS.h" // IWYU pragma: keep (configASSERT)

/**
 * @brief BSP 层统一初始化
 *
 * @note  在 osKernelInitialize() 之后、osKernelStart() 之前调用（MX_FREERTOS_Init → all_init）：
 *        此时可以创建 StreamBuffer / Mutex 等对象，但不能阻塞等待。
 *        每一条驱动的 init() 都已在下面用 configASSERT 验过，失败就地停机，因此本函数无返回值。
 *        串口的模板实例化在 bsp_uart.cpp，其余 BSP 全局实例都在本文件定义。
 *
 *        初始化顺序：DWT → UART×5 → PWM×8 → 编码器×3 → 按键；实例与绑定资源见 bsp_cfg.md。
 */
void bsp_init(void)
{
  // DWT 计时（内核 CYCCNT，不依赖 FreeRTOS/中断）：最先初始化，供其它驱动记时间戳
  configASSERT(bsp_dwt.init() == Status::OK);
  
  // UART×5（依赖 FreeRTOS 流缓冲区，失败则立刻停机）
  configASSERT(bsp_uart2.init() == Status::OK);  // USART2  PA2/PA3
  configASSERT(bsp_uart3.init() == Status::OK);  // USART3  PC10/PC11（调试打印）
  configASSERT(bsp_uart4.init() == Status::OK);  // UART4   PA11/PA12 SBUS（只收）
  configASSERT(bsp_uart7.init() == Status::OK);  // UART7   PE7/PE8
  configASSERT(bsp_uart10.init() == Status::OK); // USART10 PE2/PE3

  // PWM×8（参数取自 CubeMX；先清 CCR 再启动 → 上电全部 0% 占空比）
  configASSERT(bsp_pwm1.init() == Status::OK); // PB10 TIM2_CH3  舵机 50 Hz
  configASSERT(bsp_pwm2.init() == Status::OK); // PB11 TIM2_CH4  舵机 50 Hz
  configASSERT(bsp_pwm3.init() == Status::OK); // PB4  TIM3_CH1  20 kHz
  configASSERT(bsp_pwm4.init() == Status::OK); // PB5  TIM3_CH2  20 kHz
  configASSERT(bsp_pwm5.init() == Status::OK); // PB14 TIM12_CH1 20 kHz
  configASSERT(bsp_pwm6.init() == Status::OK); // PB15 TIM12_CH2 20 kHz
  configASSERT(bsp_pwm7.init() == Status::OK); // PB8  TIM16_CH1 20 kHz
  configASSERT(bsp_pwm8.init() == Status::OK); // PB9  TIM17_CH1 20 kHz

  // 编码器×3（只启动计数，PSC/ARR/4 倍频模式全部沿用 CubeMX）
  configASSERT(bsp_encoder1.init() == Status::OK); // PA8/PA9   TIM1
  configASSERT(bsp_encoder2.init() == Status::OK); // PD12/PD13 TIM4
  configASSERT(bsp_encoder3.init() == Status::OK); // PC6/PC7   TIM8

  // 按键（纯软件轮询消抖，200ms轮询 → 200ms消抖, 1s长按）；配置已在构造时存入
  configASSERT(key_user.init() == Status::OK);
}

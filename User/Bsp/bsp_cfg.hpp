/**
 * @file bsp_cfg.hpp
 * @author ALL
 * @brief BSP 层的全局初始化入口与全局实例声明
 * @version 0.1
 * @date 2026-04-18
 *
 * @copyright Copyright (c) 2026
 */

#ifndef __BSP_CFG_HPP__
#define __BSP_CFG_HPP__

#include "bsp_dwt.hpp"
#include "bsp_encoder.hpp"
#include "bsp_gpio.hpp"
#include "bsp_key.hpp"
#include "bsp_pwm.hpp"
#include "bsp_uart.hpp"


// ---------------- 函数声明 ----------------

/**
 * @brief BSP 层统一初始化
 *
 * @note 须在 FreeRTOS 内核启动后调用（各驱动内部要创建 RTOS 对象）。
 *       每一条驱动的 init() 都在函数内用 configASSERT 验过，失败就地停机，因此不需要返回值。
 */
void bsp_init(void);


// ----------------
// ---------------- 全局声明 ----------------

// ----------------
// ---------------- 全局实例 ----------------

// 全部实例都是 C++17 **inline 变量**：定义只写在这里一次，不再需要 .cpp 里再定义一遍，
// 也不会出现「extern 声明类型 / .cpp 定义类型不同步」这类问题。
// 新增或修改一路串口，只改本文件的一行。
//
// ⚠ .dma_buffer 落在 RAM_D1（DMA1/DMA2 访问不到 DTCM）。链接脚本里该段是**可加载段**
//   （>RAM_D1 AT> FLASH），启动时代码会先把初值拷进 RAM_D1、再跑 .init_array，
//   因此常量初始化和动态初始化都正确 —— 这是它不能写成 NOLOAD 的原因。

inline BspDwt bsp_dwt {}; ///< 内核 CYCCNT 计时（CPU 频率由 init() 从 RCC 读出，没有配置项）


// UART：{.huart = 句柄, .mode = 收发模式, .baudrate = 波特率}
// Config::baudrate 只用于估算 TX 卡死阈值；真正的波特率与帧格式由 CubeMX 写进 huartX。
// 本板 5 路串口全部是 DMA 收发，统一放在 .dma_buffer 段（DMA1/DMA2 访问不到 DTCM）。
inline __attribute__((section(".dma_buffer.bsp_uart2"))) BspUartT<uart_detail::DmaTransport, 128, 128> bsp_uart2({.huart = &huart2, .mode = UartMode::TxRx, .baudrate = 230400U}); ///< USART2 PA2/PA3

inline __attribute__((section(".dma_buffer.bsp_uart3"))) BspUartT<uart_detail::DmaTransport, 128, 128> bsp_uart3({.huart = &huart3, .mode = UartMode::TxRx, .baudrate = 115200U}); ///< USART3 PC10/PC11（调试打印）

// UART4 = SBUS：只收不发（RX 走 DMA + IDLE 断帧）；100000/8E2 由 CubeMX 写进 huart4
inline __attribute__((section(".dma_buffer.bsp_uart4"))) BspUartT<uart_detail::DmaTransport, 128, 128> bsp_uart4({.huart = &huart4, .mode = UartMode::RxOnly, .baudrate = 100000U}); ///< UART4 PA11/PA12 接收机（SBUS）

inline __attribute__((section(".dma_buffer.bsp_uart7"))) BspUartT<uart_detail::DmaTransport, 128, 128> bsp_uart7({.huart = &huart7, .mode = UartMode::TxRx, .baudrate = 921600U}); ///< UART7 PE7/PE8

inline __attribute__((section(".dma_buffer.bsp_uart10"))) BspUartT<uart_detail::DmaTransport, 128, 128> bsp_uart10({.huart = &huart10, .mode = UartMode::TxRx, .baudrate = 921600U}); ///< USART10 PE2/PE3


// GPIO 输出引脚：{.port = 端口, .pin = 引脚掩码}
inline BspGpio io1({.port = IO1_GPIO_Port, .pin = IO1_Pin}); ///< IO1 PA6
inline BspGpio io2({.port = IO2_GPIO_Port, .pin = IO2_Pin}); ///< IO2 PA7
inline BspGpio io3({.port = IO3_GPIO_Port, .pin = IO3_Pin}); ///< IO3 PC4

inline BspGpio led_b({.port = LEDB_GPIO_Port, .pin = LEDB_Pin}); ///< 蓝灯 PB0
inline BspGpio led_g({.port = LEDG_GPIO_Port, .pin = LEDG_Pin}); ///< 绿灯 PB1

inline BspGpio breaker1({.port = BREAKER1_GPIO_Port, .pin = BREAKER1_Pin}); ///< 断路器控制 PE14


// PWM 通道：{.htim, .channel, .timer_clk_hz, .prescaler, .period[, .duty_pct]}
// 本驱动不改寄存器；寄存器信息必须与 CubeMX 保持一致，否则换算是错的
// 本板定时器时钟统一 275 MHz。两组频率：舵机走 PSC=274（1 MHz 计数，方便按 us 填），
// 电机走 PSC=0（275 MHz 计数，20 kHz 时仍有 13750 级分辨率）。
// 同一定时器的两个通道共用计数器与 ARR，因此 TIM2/TIM3/TIM12 的两路必须同频。
//
// 舵机 50 Hz：ARR = 19999 → 20 ms 周期，1 计数 = 1 us，脉宽可直接按 us 填
inline BspPwm bsp_pwm1({.htim = &htim2, .channel = TIM_CHANNEL_3, .timer_clk_hz = 275000000UL, .prescaler = 274U, .period = 19999U}); ///< PB10 TIM2_CH3 舵机
inline BspPwm bsp_pwm2({.htim = &htim2, .channel = TIM_CHANNEL_4, .timer_clk_hz = 275000000UL, .prescaler = 274U, .period = 19999U}); ///< PB11 TIM2_CH4 舵机

// 电机 20 kHz PWM：PSC=0、ARR = 13749 → 275 MHz / 13750 = 20 kHz（超出可听频段，消 1 kHz 啸叫）
inline BspPwm bsp_pwm3({.htim = &htim3, .channel = TIM_CHANNEL_1, .timer_clk_hz = 275000000UL, .prescaler = 0U, .period = 13749U});  ///< PB4  TIM3_CH1
inline BspPwm bsp_pwm4({.htim = &htim3, .channel = TIM_CHANNEL_2, .timer_clk_hz = 275000000UL, .prescaler = 0U, .period = 13749U});  ///< PB5  TIM3_CH2
inline BspPwm bsp_pwm5({.htim = &htim12, .channel = TIM_CHANNEL_1, .timer_clk_hz = 275000000UL, .prescaler = 0U, .period = 13749U}); ///< PB14 TIM12_CH1
inline BspPwm bsp_pwm6({.htim = &htim12, .channel = TIM_CHANNEL_2, .timer_clk_hz = 275000000UL, .prescaler = 0U, .period = 13749U}); ///< PB15 TIM12_CH2
inline BspPwm bsp_pwm7({.htim = &htim16, .channel = TIM_CHANNEL_1, .timer_clk_hz = 275000000UL, .prescaler = 0U, .period = 13749U}); ///< PB8  TIM16_CH1
inline BspPwm bsp_pwm8({.htim = &htim17, .channel = TIM_CHANNEL_1, .timer_clk_hz = 275000000UL, .prescaler = 0U, .period = 13749U}); ///< PB9  TIM17_CH1


// 编码器：{.htim = 句柄, .invert = 方向取反}
// CubeMX 已配成 Encoder TI12（A/B 两相 4 倍频）、PSC=0、ARR=65535；这里只负责启动计数。
// 若实测方向与机构相反，把 invert 改 true（不用改接线）。
inline BspEncoder bsp_encoder1({.htim = &htim1, .invert = false}); ///< PA8/PA9   TIM1_CH1/CH2
inline BspEncoder bsp_encoder2({.htim = &htim8, .invert = false}); ///< PC6/PC7   TIM8_CH1/CH2
inline BspEncoder bsp_encoder3({.htim = &htim4, .invert = false}); ///< PD12/PD13 TIM4_CH1/CH2


// 按键：配置在构造时存入，init() 只做校验 + 读一次初始电平（要等 MX_GPIO_Init 跑过）
// KEY1 = PC5：按下为 GND（低有效）；CubeMX 配的无上下拉，靠外部上拉，没外部上拉就改 MX_GPIO_Init
inline BspKey key_user({.port = KEY1_GPIO_Port, .pin = KEY1_Pin, .active_low = true, .debounce_cnt = 1U, .long_press_cnt = 5U}); ///< 用户按键 KEY1(PC5)，按下 = GND

// ----------------


#endif // __BSP_CFG_HPP__

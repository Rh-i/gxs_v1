/**
 * @file board_cfg.hpp
 * @brief 板级实例配置（gxs_v1）
 *
 * @details 由旧工程的 `User/Bsp/bsp_cfg.hpp` + `User/Device/device_cfg.hpp` 合并而来：
 *          - 外设实例改成 C++20 `inline` 变量，定义只在这里出现一次
 *          - 旧 `UartMode::TxRx` → `UartDir::TxRx`，旧 `uart_detail::DmaTransport`
 *            → `uart_mode::dma`
 *          - 旧 `DeviceMotor` 的 `.uart` 字段新库没有，波形改用 `pid_frame()` 自己发
 *          - 旧 `Pid::Config{{kp, ki, kd}, {limit}}` → `PID::Config(kp, ki, kd, out_max)`
 *
 *          【硬规矩】
 *            - 串口用 DMA 时必须放 `.dma_buffer`
 *            - 只能用库里已有的 `BspUartT` 组合：`<dma,128,128>`、`<it,128,128>`、`<it,128,32>`
 *            - 每个实例都要在 `board_instances.hpp` 里出现，否则中断不会分发
 */

#ifndef __BOARD_CFG_HPP__
#define __BOARD_CFG_HPP__

// CubeMX 句柄
#include "main.h"
#include "tim.h"
#include "usart.h"

#include "bsp_dwt.hpp"
#include "bsp_encoder.hpp"
#include "bsp_gpio.hpp"
#include "bsp_key.hpp"
#include "bsp_pwm.hpp"
#include "bsp_uart.hpp"

#include "device_motor.hpp"
#include "device_r9ds.hpp"
#include "device_servo.hpp"


/** @brief 板级初始化：各实例 init() */
void board_init(void);


/* ==================== 外设：计时 ==================== */

inline BspDwt bsp_dwt {}; ///< 内核 CYCCNT 计时，CPU 频率由 init() 从 RCC 读出


/* ==================== 外设：UART（5 路全 DMA） ==================== */

inline __attribute__((section(".dma_buffer"))) BspUartT<uart_mode::dma, 128, 128> bsp_uart2(
    {.huart = &huart2, .mode = UartDir::TxRx, .baudrate = 230400U}); ///< PA2/PA3

inline __attribute__((section(".dma_buffer"))) BspUartT<uart_mode::dma, 128, 128> bsp_uart3(
    {.huart = &huart3, .mode = UartDir::TxRx, .baudrate = 115200U}); ///< PC10/PC11（调试打印）

// UART4 = SBUS：只收不发（RX 走 DMA + IDLE 断帧）；100000/8E2 由 CubeMX 写进 huart4
inline __attribute__((section(".dma_buffer"))) BspUartT<uart_mode::dma, 128, 128> bsp_uart4(
    {.huart = &huart4, .mode = UartDir::RxOnly, .baudrate = 100000U}); ///< PA11/PA12（SBUS）

inline __attribute__((section(".dma_buffer"))) BspUartT<uart_mode::dma, 128, 128> bsp_uart7(
    {.huart = &huart7, .mode = UartDir::TxRx, .baudrate = 921600U}); ///< PE7/PE8

inline __attribute__((section(".dma_buffer"))) BspUartT<uart_mode::dma, 128, 128> bsp_uart10(
    {.huart = &huart10, .mode = UartDir::TxRx, .baudrate = 921600U}); ///< PE2/PE3

/** @brief 调试打印：默认走 USART3 */
#define print_uart(...) bsp_uart3.printf(__VA_ARGS__)


/* ==================== 外设：GPIO / 按键 ==================== */

inline BspGpio io1({.port = IO1_GPIO_Port, .pin = IO1_Pin}); ///< PA6
inline BspGpio io2({.port = IO2_GPIO_Port, .pin = IO2_Pin}); ///< PA7
inline BspGpio io3({.port = IO3_GPIO_Port, .pin = IO3_Pin}); ///< PC4

inline BspGpio led_b({.port = LEDB_GPIO_Port, .pin = LEDB_Pin}); ///< PB0
inline BspGpio led_g({.port = LEDG_GPIO_Port, .pin = LEDG_Pin}); ///< PB1

inline BspGpio breaker1({.port = BREAKER1_GPIO_Port, .pin = BREAKER1_Pin}); ///< PE14

inline BspKey key_user({.port = KEY1_GPIO_Port, .pin = KEY1_Pin, .active_low = true,
                        .debounce_cnt = 1U, .long_press_cnt = 5U}); ///< KEY1(PC5)，按下 = GND


/* ==================== 外设：PWM ==================== */

// 频率 = timer_clk_hz / (prescaler + 1) / (period + 1)；同一定时器的两个通道必须同频
// 舵机 50 Hz：PSC=274（1 MHz 计数，1 计数 = 1 µs）、ARR=19999 → 20 ms
inline BspPwm bsp_pwm1({.htim = &htim2, .channel = TIM_CHANNEL_3, .timer_clk_hz = 275000000UL,
                        .prescaler = 274U, .period = 19999U}); ///< PB10 TIM2_CH3 舵机
inline BspPwm bsp_pwm2({.htim = &htim2, .channel = TIM_CHANNEL_4, .timer_clk_hz = 275000000UL,
                        .prescaler = 274U, .period = 19999U}); ///< PB11 TIM2_CH4 舵机

// 电机 20 kHz：PSC=0、ARR=13749 → 275 MHz / 13750 = 20 kHz
inline BspPwm bsp_pwm3({.htim = &htim3, .channel = TIM_CHANNEL_1, .timer_clk_hz = 275000000UL,
                        .prescaler = 0U, .period = 13749U}); ///< PB4  TIM3_CH1
inline BspPwm bsp_pwm4({.htim = &htim3, .channel = TIM_CHANNEL_2, .timer_clk_hz = 275000000UL,
                        .prescaler = 0U, .period = 13749U}); ///< PB5  TIM3_CH2
inline BspPwm bsp_pwm5({.htim = &htim12, .channel = TIM_CHANNEL_1, .timer_clk_hz = 275000000UL,
                        .prescaler = 0U, .period = 13749U}); ///< PB14 TIM12_CH1
inline BspPwm bsp_pwm6({.htim = &htim12, .channel = TIM_CHANNEL_2, .timer_clk_hz = 275000000UL,
                        .prescaler = 0U, .period = 13749U}); ///< PB15 TIM12_CH2
inline BspPwm bsp_pwm7({.htim = &htim16, .channel = TIM_CHANNEL_1, .timer_clk_hz = 275000000UL,
                        .prescaler = 0U, .period = 13749U}); ///< PB8  TIM16_CH1
inline BspPwm bsp_pwm8({.htim = &htim17, .channel = TIM_CHANNEL_1, .timer_clk_hz = 275000000UL,
                        .prescaler = 0U, .period = 13749U}); ///< PB9  TIM17_CH1


/* ==================== 外设：编码器（TI12 四倍频） ==================== */

inline BspEncoder bsp_encoder1({.htim = &htim1, .invert = false}); ///< PA8/PA9   TIM1
inline BspEncoder bsp_encoder2({.htim = &htim8, .invert = false}); ///< PC6/PC7   TIM8
inline BspEncoder bsp_encoder3({.htim = &htim4, .invert = false}); ///< PD12/PD13 TIM4


/* ==================== 设备实例 ==================== */

/// R9DS 遥控接收机：UART4（SBUS，DMA 接收）
inline DeviceR9dsT<decltype(bsp_uart4)> r9ds(bsp_uart4);

/// 舵机：TIM2 两路 50 Hz PWM，脉宽按 µs 填
inline DeviceServo servo1({.pwm = &bsp_pwm1, .pulse_min_us = 500.0f, .pulse_max_us = 2500.0f,
                           .range_deg = 270.0f});
inline DeviceServo servo2({.pwm = &bsp_pwm2, .pulse_min_us = 500.0f, .pulse_max_us = 2500.0f,
                           .range_deg = 180.0f, .max_speed_dps = 300.0f,
                           .max_accel_dps2 = 1500.0f});

/**
 * 电机：双 PWM（H 桥 IN1/IN2）+ 编码器 + 单套速度环
 *   机械参数：500 线编码器 × TI12(4 倍频) × 1:32 减速，轮直径 7 cm
 *   绑定：电机1 = TIM3_CH1/CH2 + 编码器1；电机2 = TIM16_CH1 / TIM17_CH1 + 编码器2；
 *         电机3 = TIM12_CH2 / TIM12_CH1 + 编码器3
 */
inline DeviceMotor motor1({.pwm_a              = &bsp_pwm3,
                           .pwm_b              = &bsp_pwm4,
                           .encoder            = &bsp_encoder1,
                           .encoder_lines      = 500.0f,
                           .encoder_multiplier = 4.0f,
                           .gear_ratio         = 32.0f,
                           .wheel_diameter_cm  = 7.0f,
                           .invert             = true, // pwm_a 实际是负方向：正输出改走 pwm_b
                           .pid                = PID::Config(18.0f, 0.016f, 0.0f, 1000.0f)});

inline DeviceMotor motor2({.pwm_a              = &bsp_pwm7,
                           .pwm_b              = &bsp_pwm8,
                           .encoder            = &bsp_encoder2,
                           .encoder_lines      = 500.0f,
                           .encoder_multiplier = 4.0f,
                           .gear_ratio         = 32.0f,
                           .wheel_diameter_cm  = 7.0f,
                           .invert             = true,
                           .pid                = PID::Config(15.0f, 0.017f, 0.0f, 1000.0f)});

inline DeviceMotor motor3({.pwm_a              = &bsp_pwm5,
                           .pwm_b              = &bsp_pwm6,
                           .encoder            = &bsp_encoder3,
                           .encoder_lines      = 500.0f,
                           .encoder_multiplier = 4.0f,
                           .gear_ratio         = 32.0f,
                           .wheel_diameter_cm  = 7.0f,
                           .pid                = PID::Config(20.0f, 0.023f, 0.0f, 1000.0f)});

#endif // __BOARD_CFG_HPP__

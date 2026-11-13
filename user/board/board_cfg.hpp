/**
 * @file board_cfg.hpp
 * @brief 板级实例配置（gxs_v1）：本板有哪些外设实例、绑定哪些 CubeMX 句柄、参数是什么
 *
 * @details
 *
 * 【板级四个文件的分工】
 *
 *   | 文件                  | 放什么                                | 什么时候改              |
 *   | --------------------- | ------------------------------------- | ----------------------- |
 *   | `hardware_config.h`   | 库的全部宏（开关/优先级/队列/超时）    | 调库的行为、换板        |
 *   | `board_cfg.hpp`（本文件） | 外设实例（inline 全局变量）与句柄绑定 | 加/删外设、改量程波特率 |
 *   | `board_instances.hpp` | 实例名清单（X-macro）                 | **每次加实例都要同步**  |
 *   | `board_cfg.cpp`       | `board_init()`：谁先 init、谁后 init  | 改初始化顺序、加设备    |
 *
 * 【三条硬规矩（踩了就很难查）】
 *
 *   1. **串口 / SPI 实例必须放 `.dma_buffer` 段，且段名按变量唯一**
 *      H7 的 DMA1/DMA2 访问不到 ITCM / DTCM；段名必须写成 `.dma_buffer.<变量名>`，
 *      因为实例是 C++17 `inline` 变量，多个 TU 各 emit 一份弱定义。
 *      `ICBSP_DMA_BUFFER(变量名)` 就是干这个的。
 *
 *   2. **`BspUartT` 只有头文件，Transport 与缓冲尺寸随便组合**
 *      DMA 模式的 RX DMA 由 `init()` 自动改成循环模式；CubeMX 里配 Normal 也可以。
 *
 *   3. **本文件每加一个实例，`board_instances.hpp` 必须同步加名字**
 *      串口实例漏写时 `init()` 返回 BAD_ARG（board_init() 的 ICBSP_CHECK 会停在那行）。
 *
 * 【本板（gxs_v1 / DM-MC02 类）】
 *   无 CAN、无 SPI；UART：2/3/4/7/10（USART3 是调试打印）；
 *   3 路 DeviceMotor（TIM3 / TIM16+TIM17 / TIM12 + TIM1/TIM8/TIM4 编码器）+ 2 路舵机
 *   + R9DS(SBUS, UART4) + WT102 陀螺仪(UART2)。
 *
 * @note 本工程是手写板级配置，不使用 integrate.py 的 `<ICBSP-GEN ...>` 生成区。
 */

#ifndef __BOARD_CFG_HPP__
#define __BOARD_CFG_HPP__

// CubeMX 生成的句柄头文件
#include "main.h"  // IWYU pragma: keep
#include "tim.h"   // IWYU pragma: keep
#include "usart.h" // IWYU pragma: keep

#include "bsp/bsp_series.hpp" // IWYU pragma: keep（ICBSP_DMA_BUFFER、ICBSP_CAN_MODULE_ENABLED）
#include "bsp/bsp_dwt.hpp"
#include "bsp/bsp_encoder.hpp"
#include "bsp/bsp_gpio.hpp"
#include "bsp/bsp_key.hpp"
#include "bsp/bsp_pwm.hpp"
#include "bsp/bsp_uart.hpp"

#include "device/actuator/device_servo.hpp"
#include "device/motor/device_motor.hpp"
#include "device/remote/device_r9ds.hpp"
#include "device/sensor/device_wt102.hpp"


/** @brief 板级初始化：各实例 init()，实现在 board_cfg.cpp */
void board_init(void);


/* ==================== 外设实例 ==================== */
// 外设实例是 inline 全局变量：整个工程只有一份定义，任何包含本头文件的 TU 都能直接用。
// 为什么必须 constexpr 构造 + 平凡析构：inline 变量要求常量初始化，否则每个 TU 都会生成
// 一份构造代码；需要释放资源时显式调 deinit()。

/**
 * DWT 计时基准：**由库内部初始化**
 *
 * @note 实例定义在 `code/service/system/system.cpp`，这里只 `#include "bsp/bsp_dwt.hpp"`
 *       后直接调用（如 `bsp_dwt.get_delta_s(&last)`）。不要在这里再定义实例、也不要再调 init()。
 */

/* ==================== 外设：UART（5 路全 DMA） ==================== */

inline ICBSP_DMA_BUFFER(bsp_uart2) BspUartT<uart_mode::dma, 128, 128> bsp_uart2(
  {.huart = &huart2, .mode = UartDir::TxRx, .baudrate = 230400U}); ///< PA2/PA3（WT102）

inline ICBSP_DMA_BUFFER(bsp_uart3) BspUartT<uart_mode::dma, 128, 128> bsp_uart3(
  {.huart = &huart3, .mode = UartDir::TxRx, .baudrate = 115200U}); ///< PC10/PC11（调试打印）

// UART4 = SBUS：只收不发（RX 走 DMA + IDLE 断帧）；100000/8E2 由 CubeMX 写进 huart4
inline ICBSP_DMA_BUFFER(bsp_uart4) BspUartT<uart_mode::dma, 128, 128> bsp_uart4(
  {.huart = &huart4, .mode = UartDir::RxOnly, .baudrate = 100000U}); ///< PA11/PA12（SBUS）

inline ICBSP_DMA_BUFFER(bsp_uart7) BspUartT<uart_mode::dma, 128, 128> bsp_uart7(
  {.huart = &huart7, .mode = UartDir::TxRx, .baudrate = 921600U}); ///< PE7/PE8

inline ICBSP_DMA_BUFFER(bsp_uart10) BspUartT<uart_mode::dma, 128, 128> bsp_uart10(
  {.huart = &huart10, .mode = UartDir::TxRx, .baudrate = 921600U}); ///< PE2/PE3

/** @brief 调试打印：默认走 USART3；关掉时宏展开为空，调用点不用改 */
#define ICBSP_UART_PRINT_ENABLE 1

#if ICBSP_UART_PRINT_ENABLE
#  define print_uart(...) bsp_uart3.printf(__VA_ARGS__)
#else
#  define print_uart(...) ((void)0)
#endif


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


/* ==================== */
/* ==================== 设备实例 ==================== */
// 设备层：包装外设，提供“按物理量”的接口；顺序要求：必须在上面的外设实例之后定义，
// 并且在 board_init() 里外设 init() 之后 init()。

/// R9DS 遥控接收机：UART4（SBUS，DMA 接收）
inline DeviceR9dsT<decltype(bsp_uart4)> r9ds(bsp_uart4);

/// WT102 单轴陀螺仪：UART2（230400 8N1，上电即以 500 Hz 主动上报 0x52/0x53 帧）
inline DeviceWt102T<decltype(bsp_uart2)> wt102(bsp_uart2);

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

/* ==================== */

#endif // __BOARD_CFG_HPP__

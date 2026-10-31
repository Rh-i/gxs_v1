/**
 * @file device_cfg.hpp
 * @author Rh
 * @brief 设备层统一管理 —— device_init() 与全局实例声明
 * @version 0.3
 * @date 2026-10-04
 *
 * @copyright Copyright (c) 2026
 *
 * @note 设备实例在此声明、在 device_cfg.cpp 中实例化；本层的初始化动作全部收进
 *       device_init()，与 bsp_init() / service_init() 对齐。
 */

#ifndef __DEVICE_CFG_HPP__
#define __DEVICE_CFG_HPP__

#include "bsp_cfg.hpp"
#include "device_motor.hpp"
#include "device_r9ds.hpp"
#include "device_servo.hpp"

// ---------------- 函数声明 ----------------

/**
 * @brief 逐个初始化本层设备
 *
 * @note 设备自己不写全局状态；初始化失败在这里用 configASSERT 就地停机，因此不需要返回值。
 */
void device_init(void);

// ----------------
// ---------------- 全局声明 ----------------

// 新增设备要改三处：这里的 extern → device_cfg.cpp 的实例定义 →
// 对应设备 .cpp 顶部的显式模板实例化（串口类型必须与 bsp_cfg.hpp 一致）。
extern DeviceR9dsT<decltype(bsp_uart4)> r9ds; ///< R9DS 遥控接收机（UART4，DMA 接收，SBUS）

/// 舵机：绑定 BSP 层 TIM2 的 2 路 50 Hz PWM（bsp_pwm1/2 = PB10 / PB11）
extern DeviceServo servo1; ///< PB10 TIM2_CH3
extern DeviceServo servo2; ///< PB11 TIM2_CH4

/// 电机：双 PWM（H 桥 IN1/IN2）+ 编码器 + 单套速度环。
/// pwm_a / pwm_b / encoder 三个指针**互不相关**，按实际接线填（见 device_cfg.cpp）。
extern DeviceMotor motor1; ///< 三路绑定未定：由使用者自己填
extern DeviceMotor motor2; ///< 三路绑定未定：由使用者自己填
extern DeviceMotor motor3; ///< 三路绑定未定：由使用者自己填

// ----------------
#endif // __DEVICE_CFG_HPP__

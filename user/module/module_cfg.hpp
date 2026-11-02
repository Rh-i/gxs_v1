/**
 * @file module_cfg.hpp
 * @author Rh
 * @brief Module 层统一管理 —— module_init() 与全局实例声明
 * @version 0.1
 * @date 2026-10-29
 *
 * @copyright Copyright (c) 2026
 *
 * @note 全局实例在此声明、在 module_cfg.cpp 中实例化；本层的初始化动作收进 module_init()，
 *       与 bsp_init() / device_init() 对齐，由 all_init() 在设备层之后调用。
 */

#ifndef __MODULE_CFG_HPP__
#define __MODULE_CFG_HPP__

#include "omni3_chassis.hpp"


/**
 * @brief Module 层统一初始化
 *
 * @note 内部用 configASSERT 校验配置，失败就地停机，因此不需要返回值。
 *       须在 device_init() 之后调用（依赖三个已绑定的 DeviceMotor）。
 */
void module_init(void);


// ----------------
// ---------------- 全局声明 ----------------

extern Omni3Chassis chassis; ///< 三全向轮底盘（M1 右前 / M2 左前 / M3 正后）

// ----------------
#endif // __MODULE_CFG_HPP__

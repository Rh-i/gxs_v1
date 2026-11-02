#include "module_cfg.hpp"

#include "board_cfg.hpp" // motor1/2/3


// 轮心到车体中心的距离 (cm)：**待实测** —— 量某个轮子的接触点到三轮几何中心的距离。
// 只影响自转：给 2 rad/s 时实际转多快，误差比例就等于 R 的误差比例；平移不受它影响。
constexpr float CHASSIS_R_CM = 10.0f;

// 底盘限幅：要盖住遥控侧的上限（平移 120 cm/s、自转 120/10 = 12 rad/s），
// 否则会在这里被静默截断，表现为「摇杆推满也不见得快」。
constexpr float CHASSIS_V_MAX_CM_S  = 130.0f;
constexpr float CHASSIS_W_MAX_RAD_S = 13.0f;


// ---------------- 全局实例 ----------------

Omni3Chassis chassis({.motor = {&motor1, &motor2, &motor3},
                      .wheel_radius_cm = CHASSIS_R_CM,
                      .v_max_cm_s      = CHASSIS_V_MAX_CM_S,
                      .w_max_rad_s     = CHASSIS_W_MAX_RAD_S});


// ----------------
// ---------------- 函数定义 ----------------


/** @brief Module 层统一初始化：失败就地停机 */
void module_init(void)
{
  chassis.init();
}

// ----------------

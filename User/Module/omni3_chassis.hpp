/**
 * @file omni3_chassis.hpp
 * @author Rh
 * @brief 三全向轮底盘运动学 —— (v_x, v_y, ω) → 三个轮子的目标线速度
 * @version 0.1
 * @date 2026-10-29
 *
 * @copyright Copyright (c) 2026
 *
 * @details 三个全向轮互隔 120°，**前向 = M1/M2 的中线**，方位角（从 +x 逆时针）：
 *          M1 右前 30°、M2 左前 150°、M3 正后 270°。
 *          实测电机「正转」驱动底盘的方向是 d_i = (-sinθ_i, +cosθ_i)，于是
 *
 *              v_i = a_i·v_x + b_i·v_y + R·ω            （cm/s、cm、rad/s）
 *              a = { -0.500, -0.500, +1.000 }
 *              b = { +0.866, -0.866,  0.000 }
 *
 *          ω 俯视**逆时针为正**。三轮的自转系数相同（都是 +R·ω），这是 120° 对称布局的
 *          性质：自转时三轮同速同向，因此自转不受轮速比例误差影响，只与 R 的标定有关。
 *
 *          ⚠ 2026-10-29 实测：电机正转方向与最初的图上箭头相反，故系数整体取反。
 *            若只有单个轮子装反，改那台的 DeviceMotor::invert，不要动这里的系数。
 *
 *          R = 轮心到车体中心的距离。它只影响自转，平移不需要它；R 填错只会让「给 2 rad/s
 *          实际转得偏快/偏慢」。系数随手画出图核对：Docs/omni3/omni3_layout.py。
 *
 * @note 本类只算目标、只写 DeviceMotor 的目标值；速度环、编码器、PWM 都在下层，
 *       由 motor_task 每拍推进，所以这里没有 update()。
 */

#ifndef __OMNI3_CHASSIS_HPP__
#define __OMNI3_CHASSIS_HPP__

#include "device_motor.hpp" // 三个轮子的速度环

#include <stdint.h>


/**
 * @brief 三全向轮底盘
 *
 * @note 单写者约定：drive() / stop() 可以在任意任务调用（内部只写目标值），
 *       但同一时刻只应有一个任务在调，否则目标会互相覆盖。
 */
class Omni3Chassis
{
public:
  // ---------------- 配置 ----------------

  /** @brief 底盘配置 */
  struct Config
  {
    DeviceMotor *motor[3] = {nullptr, nullptr, nullptr}; ///< 顺序固定：M1 右前 / M2 左前 / M3 正后
    float        wheel_radius_cm = 0.0f;                 ///< 轮心到车体中心距离 R (cm)，只用于自转项
    float        v_max_cm_s      = 100.0f;               ///< 每轮目标限幅 (cm/s)
    float        w_max_rad_s     = 2.0f;                 ///< 自转角速度限幅 (rad/s)
  };

  // ----------------
  // ---------------- 公有接口 ----------------

  /** @brief 默认构造：未绑定，init() 会断言停机 */
  Omni3Chassis() = default;

  /** @brief 构造：只保存配置（不碰硬件） */
  explicit constexpr Omni3Chassis(const Config &cfg) : _config(cfg) {}

  /**
   * @brief 校验配置并让三个轮子先停住
   *
   * @note 电机没绑或 R 没填属于配置错误，直接 configASSERT 停机；
   *       参数合法时本函数不做别的事，速度环仍由 motor_task 推进。
   */
  void init();

  /**
   * @brief 设定底盘速度（平移 + 自转，可叠加）
   *
   * @param vx_cm_s  右为正 (cm/s)
   * @param vy_cm_s  前为正 (cm/s)
   * @param wz_rad_s 俯视逆时针为正 (rad/s)
   *
   * @note 先按 Config 的 w_max 限自转，再对每个轮子的合成目标按 v_max 限幅：
   *       平移与自转同时给满时，轮速是两者之和，可能超过单个电机的可用速度。
   */
  void drive(float vx_cm_s, float vy_cm_s, float wz_rad_s);

  /**
   * @brief 三个轮子真正停机
   *
   * @note 走 DeviceMotor::stop()：输出归 0、模式回 STOP、清积分。
   *       与 drive(0,0,0) 不同：后者保持 SPEED 模式，零速时会因编码器抖动而一直嗡嗡响。
   */
  void stop();

  /**
   * @brief 最近一次下发给某个轮子的目标速度
   * @param index 0=M1 1=M2 2=M3
   * @return 目标速度 (cm/s)
   *
   * @note 给测试对比「目标 / 实测」用；越界下标返回 0（下标来自本类内部，不做断言）。
   */
  float wheel_target(uint8_t index) const { return (index < 3U) ? _target[index] : 0.0f; }

  // ----------------
private:
  // ---------------- 成员变量 ----------------

  Config _config;              ///< 绑定与限幅
  float  _target[3] = {};      ///< 最近一次下发的三轮目标 (cm/s)

  // ----------------
};

#endif // __OMNI3_CHASSIS_HPP__

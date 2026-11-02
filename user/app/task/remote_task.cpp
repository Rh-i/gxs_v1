/**
 * @file remote_task.cpp
 * @brief 遥控任务：R9DS 通道 → 底盘速度，含模式切换与失联保护
 * @date 2026-10-29
 *
 * 通道表（Rc 语义值 -100~+100，开关给挡位）：
 *   CH1 右摇杆上下 → 自旋，正 = 左旋（俯视逆时针）
 *   CH3 左摇杆上下 → 前进/后退，正 = 前进
 *   CH4 左摇杆左右 → 左右平移，正 = 左移
 *   CH5 C 三挡开关 → 模式：+100 自动（接口暂留）/ 0 停车 / -100 遥控
 *   CH6~CH8 旋钮推杆、CH9/CH10 B/A 开关：预留
 *
 * 新增通道的做法：常量区加一个比例，`mode < 0` 分支里多读一个 `rc.xxx`，
 * 再在 Module 层加对应的目标接口；不要让本任务直接碰 PWM 或电机。
 *
 * 安全约束：链路不可用或 CH5 不在遥控挡时一律刹车，不复用上一拍的摇杆值。
 * 本任务独占 `r9ds.update()`，不能再有第二个消费者。
 */

#include "app_task.hpp"

#include "board_cfg.hpp" // r9ds
#include "module_cfg.hpp" // chassis

#include "FreeRTOS.h" // IWYU pragma: keep
#include "task.h"

#include <stdint.h>


constexpr uint32_t REMOTE_PERIOD_MS  = 10U;    // 100 Hz：摇杆变化远慢于此，再快只是重复写目标
constexpr float    REMOTE_V_MAX_CM_S = 120.0f; // 摇杆打满 = 120 cm/s
// 自转满杆取「轮速与平移满杆一致」：ω_max = V_MAX / R = 120 / 10，R 见 module_cfg.cpp
// 原来写 2 rad/s 时轮速只要 20 cm/s，所以自转看起来比平移慢一大截
constexpr float   REMOTE_W_MAX_RAD_S = 12.0f;
constexpr int16_t STICK_DEADBAND     = 5;     // 遥控器语义值 ±100 的 5%，滤掉中位零漂


/** @brief 摇杆 → -1.0~+1.0，中位死区内直接给 0 */
static float stick(int16_t value)
{
  if ((value <= STICK_DEADBAND) && (value >= -STICK_DEADBAND))
    return 0.0f;

  return static_cast<float>(value) / 100.0f;
}


extern "C" void remote_task(void *argument)
{
  (void)argument;

  TickType_t wake_time = xTaskGetTickCount();

  for (;;)
  {
    r9ds.update(); // 本任务是该串口唯一的接收消费者

    // 链路不可用（拔线 / 遥控器关机 / 连续丢帧）直接刹车，不接受残余摇杆值
    if (!r9ds.is_online())
    {
      chassis.stop();
      vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(REMOTE_PERIOD_MS));
      continue;
    }

    const auto  &rc   = r9ds.rc();
    const int8_t mode = static_cast<int8_t>(rc.switch_c); // +1 自动 / 0 停车 / -1 遥控

    if (mode > 0)
    {
      // 自动模式：后续由视觉/上位机在 Module 层给出目标；接上之前保持刹车
      chassis.stop();
    }
    else if (mode < 0)
    {
      const float fwd  = stick(rc.left_vertical);   // +1 前进
      const float left = stick(rc.left_horizontal); // +1 左移
      const float ccw  = stick(rc.right_vertical);  // +1 左旋

      // 底盘约定 v_x 右正、v_y 前正、ω 逆时针正，所以「左移」要取负
      chassis.drive(-left * REMOTE_V_MAX_CM_S, fwd * REMOTE_V_MAX_CM_S, ccw * REMOTE_W_MAX_RAD_S);
    }
    else
    {
      chassis.stop(); // CH5 中位 = 停车，无视摇杆
    }

    vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(REMOTE_PERIOD_MS));
  }
}

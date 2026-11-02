/**
 * @file omni3_test.cpp
 * @brief 三全向轮底盘运动学自测：按相位表逐段给 (v_x, v_y, ω)，打印三轮目标/实测
 * @date 2026-10-29
 *
 * 前置：motor_task 已在跑（1 kHz 速度环），本任务只改目标、不碰 PWM/编码器。
 * 期望打印（每段 2 s，200 ms 一行）：
 *   [OMNI] FWD   tgt   +17   -17    +0 | act   +16.8   -17.1    +0.2
 *   [OMNI] SPIN+ tgt   +20   +20   +20 | act   +19.5   +20.2   +19.8
 * 判据：FWD 时 M1/M2 反向、M3 为 0；SPIN+ 时三轮同号同值；act 在 1 s 内跟上 tgt。
 * ⚠ 底盘会真的动：先架空轮子跑一遍，确认三轮转向与图示一致再落地。
 */

#include "app_test.hpp"

#include "board_cfg.hpp"    // bsp_uart3（打印）
#include "module_cfg.hpp" // chassis

#include "FreeRTOS.h" // IWYU pragma: keep
#include "task.h"

#include <stdint.h>



// 相位表：vx 右正、vy 前正、ω 俯视逆时针正（cm/s、rad/s）
constexpr struct
{
  const char *name;
  float       vx;
  float       vy;
  float       w;
} PHASES[] = {
  {"STOP", 0.0f, 0.0f, 0.0f},
  {"FWD", 0.0f, 20.0f, 0.0f},
  {"BACK", 0.0f, -20.0f, 0.0f},
  {"RIGHT", 20.0f, 0.0f, 0.0f},
  {"LEFT", -20.0f, 0.0f, 0.0f},
  {"SPIN+", 0.0f, 0.0f, 2.0f},
  {"SPIN-", 0.0f, 0.0f, -2.0f},
};

constexpr uint32_t PHASE_NUM = sizeof(PHASES) / sizeof(PHASES[0]);
constexpr uint32_t PHASE_MS  = 2000U; // 每段 2 s：留够速度环把阶跃收敛掉
constexpr uint32_t PRINT_MS  = 200U;  // 打印 5 Hz，够看趋势又不刷爆 115200


extern "C" void omni3_test_task(void *argument)
{
  (void)argument;
  static_assert(configTICK_RATE_HZ == 1000U, "本文件的毫秒常量按 1 ms tick 写");

  bsp_uart3.printf("[OMNI] 3-wheel omni: FWD=+y, RIGHT=+x, SPIN+=CCW\r\n");

  for (uint32_t t = 0U;; t += PRINT_MS)
  {
    const auto &p = PHASES[(t / PHASE_MS) % PHASE_NUM];

    // STOP 段走真正停机（模式回 STOP、输出恒 0）；其余段由速度环跟随
    // motor_task 每拍推进，这里重复下发同一个目标没有副作用
    if ((p.vx == 0.0f) && (p.vy == 0.0f) && (p.w == 0.0f))
      chassis.stop();
    else
      chassis.drive(p.vx, p.vy, p.w);

    bsp_uart3.printf("[OMNI] %-5s tgt %+5.0f %+5.0f %+5.0f | act %+5.1f %+5.1f %+5.1f\r\n", p.name,
                     chassis.wheel_target(0), chassis.wheel_target(1), chassis.wheel_target(2),
                     motor1.speed_cm_s(), motor2.speed_cm_s(), motor3.speed_cm_s());

    vTaskDelay(PRINT_MS);
  }
}


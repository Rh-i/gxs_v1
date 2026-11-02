#include "app_task.hpp"
#include "board_cfg.hpp"

#include "FreeRTOS.h" // IWYU pragma: keep
#include "task.h"

#include <stdint.h>


/** @brief 控制周期 (ms)：1 ms = 1 kHz。想更快就改成别的值，update() 里的 dt 是实测的，不用同步改 */
constexpr uint32_t MOTOR_TASK_PERIOD_MS = 1U;


/**
 * @brief 电机速度环任务：按 MOTOR_TASK_PERIOD_MS 周期跑三路电机的 update()
 *
 * @note dt 用 bsp_dwt 实测（秒）而不是名义周期：任务被抢占/抖动时，PID 的 I、D 仍然对得上真实时间。
 * @note 只处理已 init 的电机：没填绑定的电机直接跳过，不会碰到空指针。
 */
extern "C" void motor_task(void *argument)
{
  (void)argument;

  TickType_t wake_time = xTaskGetTickCount();

  // 时间戳初值无意义：第一次 get_delta_s() 的结果丢掉
  uint32_t mark = 0U;
  bsp_dwt.get_delta_s(&mark);

  for (;;)
  {
    const float dt_s = static_cast<float>(bsp_dwt.get_delta_s(&mark));

      motor1.update(dt_s);
      motor2.update(dt_s);
      motor3.update(dt_s);

    vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(MOTOR_TASK_PERIOD_MS));
  }
}

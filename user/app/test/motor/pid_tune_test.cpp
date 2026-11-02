#include "app_test.hpp"

#include "board_cfg.hpp"

#include "FreeRTOS.h" // IWYU pragma: keep
#include "task.h"

#include <stdint.h>


// 方波给定：±30 cm/s，半周期 10 s（想快点就改 TUNE_HALF_S）
constexpr float TUNE_SPEED  = 100.0f;
constexpr float TUNE_HALF_S = 1.0f;

/**
 * @brief PID 调参任务：1 kHz 跑控制环，只让 motor3 跟一个 ±100 cm/s 的方波
 *
 * @note 现在只刷新 / 只调 / 只打印 motor3；
 * @note 每 1 ms 发一帧 JustFloat VOFA+ 选 JustFloat 协议就能看阶跃响应。
 */
extern "C" void pid_tune_task(void *argument)
{
  (void)argument;

  uint32_t mark = 0U;
  bsp_dwt.get_delta_s(&mark); // 首次结果丢掉

  float    target = TUNE_SPEED; // 当前方波电平
  float    timer  = 0.0f;       // 半周期计时
  uint32_t n      = 0U;         // 打印分频计数

  motor3.set_speed(target);

  for (;;)
  {
    const float dt_s = static_cast<float>(bsp_dwt.get_delta_s(&mark));

    // 方波：半周期到就翻转给定
    timer += dt_s;
    if (timer >= TUNE_HALF_S)
    {
      timer -= TUNE_HALF_S;
      target = -target;
      motor3.set_speed(target);
    }

    // 只打印 motor3：降频到 200 Hz（1 kHz 满发会超过 USART3 的 115200 带宽）
    if (++n >= 5U)
    {
      n = 0U;
      uint8_t frame[16];
      const size_t len = motor3.pid_frame(frame);
      bsp_uart3.send(frame, len, 0U); // 0 ms：满就丢，不阻塞
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

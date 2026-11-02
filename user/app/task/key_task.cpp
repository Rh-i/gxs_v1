#include "board_cfg.hpp"
#include "bsp_key.hpp" // IWYU pragma: keep (BspKey::Event)
#include "FreeRTOS.h" // IWYU pragma: keep
#include "task.h"

// 按钮显示：按下亮左（LEDB=PB0）、按到长按切右（LEDG=PB1）、松手全灭
// ⚠ 若两灯接反，把下面 led_b / led_g 对调即可
extern "C" void key_task(void *argument)
{
  (void)argument;

  for (;;)
  {
    // 200ms 轮询（对应 key_user 的 debounce 1 / long_press 5 ⇒ 200ms 消抖 / 1s 长按）
    switch (key_user.update())
    {
      case BspKey::Event::PRESS: // 按下：左亮
        led_b.set();
        break;
      case BspKey::Event::LONG: // 长按：左灭、切到右
        led_b.reset();
        led_g.set();
        break;
      default:
        break;
    }

    if (!key_user.pressed()) // 松手：都灭
    {
      led_b.reset();
      led_g.reset();
    }

    vTaskDelay(200); // 固定 200 ms 轮询周期
  }
}

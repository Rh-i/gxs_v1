#include "app_task.hpp"
#include "bsp_cfg.hpp"

#include "FreeRTOS.h" // IWYU pragma: keep
#include "online_check.hpp"
#include "task.h"

#include <stdint.h>


// 维护周期
constexpr uint32_t SYS_TASK_PERIOD_MS = 10U;

// 串口组（DMA）：本板 5 路串口全部 DMA 收发，统一巡检
BspUartT<uart_detail::DmaTransport, 128, 128> *const patrol_uarts[] = {&bsp_uart2, &bsp_uart3, &bsp_uart4, &bsp_uart7, &bsp_uart10};


extern "C" void sys_task(void *argument)
{
  TickType_t wake_time = xTaskGetTickCount();

  // 降低打印次数的临时变量
  uint8_t count = 0;

  for (;;)
  {
    count++;

    // 本轮计时基准（无状态，首次结果丢弃）
    uint32_t mark = 0U;
    bsp_dwt.get_delta_s(&mark);

    // UART 补救：卡死的发送补发 + 接收通道重建（ISR 只计数，恢复只能在任务侧做）
    for (auto *uart : patrol_uarts)
    {
      uart->tx_recover();
      uart->rx_recover();
    }

    // 在线状态更新（各节点由消费该串口的任务在 update() 里刷新）
    Online::update();

    // 打印占用时间降低打印次数
    if (count == 200)
    {
      // 绝对时间：每次打印都读一次，顺带满足 DWT 绝对时间轴 < 7.809 s 的心跳要求
      const double uptime_s = bsp_dwt.get_time_s();

      // 本轮耗时（µs）：从循环开头到这里，经 USART3 打印
      // ⚠ 两个参数都必须先显式转成 unsigned 再交给 %u：vsnprintf 按格式串取变参，
      //   把 double 直接配 %d/%u 会读错槽位，打出来的是 double 的位模式（全是乱数，还会为负）
      bsp_uart3.printf("[sys] time=%us cycle=%uus\r\n",
                       static_cast<unsigned>(uptime_s),
                       static_cast<unsigned>(bsp_dwt.get_delta_s(&mark) * 1000000.0));
      count = 0;
    }

    vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(SYS_TASK_PERIOD_MS));
  }
}

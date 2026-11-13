/**
 * @file imu_task.cpp
 * @brief 陀螺仪任务：1 kHz 读 WT102（USART2），把 Z 轴三个量的最近两帧刷新到 wt102 里
 * @date 2026-11-06
 *
 * 类里只存数据，不打印、不换算：
 *   `wt102.get_z_newest()`   → 最新一帧的 {acc_z, gyro_z, yaw}（原始 int16）
 *   `wt102.get_z_previous()` → 上一帧的同样三个值
 * 换算由用的人自己做：`yaw = yaw / 32768 * 180 (°)`、`wz = gyro_z / 32768 * 2000 (°/s)`、
 * `az = acc_z / 32768 * 16 (g)`（实测静止平放 az≈-2070 → -1.01 g）。
 *
 * 本任务独占 `wt102.update()`（串口流缓冲区只允许一个读者），不能再有第二个消费者。
 */

#include "app_task.hpp"

#include "board_cfg.hpp" // wt102、bsp_uart3（打印）
#include "service/system/system.hpp"    // sys_state

#include "FreeRTOS.h" // IWYU pragma: keep
#include "task.h"

#include <stdint.h>

/** @brief 读取周期 (ms)：1 kHz，500 Hz 的数据每拍只会攒 1~2 帧，不会积压 */
constexpr uint32_t IMU_TASK_PERIOD_MS = 2U;

extern "C" void imu_task(void *argument)
{
  (void)argument;
  sys_state.wait_running(); // 初始化失败时任务自行退出，不跑在半初始化的系统上

  static_assert(configTICK_RATE_HZ == 1000U, "本文件的周期常量按 1 ms tick 写");

  TickType_t wake_time = xTaskGetTickCount();

  for (;;)
  {
    // 本任务是该串口唯一的接收消费者；结果在 wt102.get_z_newest() / z_previous()
    wt102.update();

    vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(IMU_TASK_PERIOD_MS));
  }
}

/**
 * @file app_main.cpp
 * @author Rh
 * @brief 应用入口：库入口 → 板级初始化 → 模块校验 → 放行 → 创建业务任务
 *
 * @note 由 freertos.c 的 `MX_FREERTOS_Init()`（USER CODE Init 段）调用，调度器启动前执行。
 *
 * @copyright Copyright (c) 2026
 */

#include "app_main.h"

#include "main.h" // IWYU pragma: keep
#include "task.h"

#include "board_cfg.hpp"  // 板级实例（bsp_uart3 打印、motor1/2/3）
#include "module_cfg.hpp" // chassis
#include "system.hpp"     // system_init / board_init 的 sys_state

#include "app_task.hpp"


/**
 * @brief 应用初始化
 *
 * @note 初始化顺序：
 *         1. system_init()  —— 库入口：建 sys_state，创建 sys_task（10 ms）
 *         2. board_init()   —— BSP 外设 + 设备实例 init()
 *         3. module_init()  —— 模块层（依赖已绑定的 DeviceMotor）
 *         4. system_start() —— 置运行位，放行所有 wait_running() 的任务
 *         5. 创建业务任务   —— 必须在 system_start() 之后，否则任务被立刻放行
 *
 *       每一步内部都带 configASSERT，失败就地停机，fault_record 记下行号。
 *       自测任务（user/app/test/）默认不创建，需要时在末尾自己加 xTaskCreate。
 */
void app_init(void)
{
  /* 1. 库入口：sys_state + sys_task（10 ms）。调度器还没启动，这里只是创建 */
  system_init();

  /* 2. 板级：各外设 init()、设备 init()（每路 configASSERT） */
  board_init();

  /* 3. 模块层：三全向轮底盘（依赖三个已绑定的 DeviceMotor） */
  module_init();

  /* 4. 放行：置运行位。业务任务入口的 wait_running() 在这之后才通过 */
  system_start();

  /* 5. 业务任务 */
  /* 按键任务：200 ms 轮询（短按/长按做不同提示，优先级 +5） */
  configASSERT(xTaskCreate(key_task, "key", 256, NULL, tskIDLE_PRIORITY + 5, NULL) == pdPASS);

  /* 电机任务：1 kHz 速度环（周期在 motor_task.cpp 里改，dt 由 DWT 实测） */
  configASSERT(xTaskCreate(motor_task, "motor", 512, NULL, tskIDLE_PRIORITY + 7, NULL) == pdPASS);

  /* 遥控任务：100 Hz，读 R9DS → 底盘目标（独占 r9ds.update()） */
  configASSERT(xTaskCreate(remote_task, "remote", 512, NULL, tskIDLE_PRIORITY + 5, NULL) == pdPASS);
}


/**
 * @brief 默认任务（CubeMX 生成，保留原名）
 * @param argument 任务参数
 */
extern "C" void StartDefaultTask(void *argument)
{
  (void)argument;
  for (;;)
  {
    vTaskDelay(100);
  }
}

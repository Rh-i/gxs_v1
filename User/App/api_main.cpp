#include "api_main.h"
#include "FreeRTOS.h" // IWYU pragma: keep
#include "main.h"     // IWYU pragma: keep
#include "stdio.h"
#include "task.h"


/* BSP */
#include "bsp_cfg.hpp"

/* Device */
#include "device_cfg.hpp" // IWYU pragma: keep

/* Module */
#include "module_cfg.hpp"

/* Service */
#include "service_cfg.hpp"

/* 任务声明 */
#include "app_task.hpp"

/* 测试开关与测试任务 */
#include "app_test.hpp"


/**
 * @brief FreeRTOS后的相关初始化
 *
 * @note 在main.c的MX_FREERTOS_INIT函数中调用
 *       用于创建FreeRTOS任务和初始化外设驱动
 *
 *       初始化顺序：
 *         1. service_init()  —— Service 层（系统状态标志）
 *         2. bsp_init()      —— BSP 层外设初始化
 *         3. device_init()   —— 设备层初始化
 *         4. 创建各任务      —— 任务入口先等运行态
 *         5. sys_state.complete_init() —— 置运行位
 */
void all_init()
{
  /* 各层 init 内部自带 configASSERT，失败就地停机，因此这里直接调用、不需要再验一次 */

  /* Service 层：系统状态标志，须最先（后面各层要靠它上报失败） */
  service_init();

  /* BSP 层：外设（UART / PWM / GPIO / 按键） */
  bsp_init();

/* 设备层：BSP 之后 */
  device_init();

  /* 模块层：多设备组合（三全向轮底盘） */
  module_init();

  /* 维护任务：sys_task 为 10 ms（Online 计时 + UART 断链兜底，优先级 +7） */
  configASSERT(xTaskCreate(sys_task, "sys", 256, NULL, tskIDLE_PRIORITY + 7, NULL) == pdPASS);

  /* 按键任务：200 ms 轮询（短按/长按做不同提示，优先级 +5） */
  configASSERT(xTaskCreate(key_task, "key", 256, NULL, tskIDLE_PRIORITY + 5, NULL) == pdPASS);

  /* 电机任务：1 kHz 速度环（周期在 motor_task.cpp 里改，dt 由 DWT 实测） */
  configASSERT(xTaskCreate(motor_task, "motor", 512, NULL, tskIDLE_PRIORITY + 7, NULL) == pdPASS);

  /* 遥控任务：100 Hz，读 R9DS → 底盘目标（独占 r9ds.update()） */
  configASSERT(xTaskCreate(remote_task, "remote", 512, NULL, tskIDLE_PRIORITY + 5, NULL) == pdPASS);

#if APP_TEST_MOTOR_OPENLOOP
  /* 三电机开环正反转：20 ms 一步（此模式下别开 motor_task） */
  configASSERT(xTaskCreate(motor_openloop_test_task, "open", 512, NULL, tskIDLE_PRIORITY + 4, NULL) == pdPASS);
#endif

#if APP_TEST_PID_TUNE
  /* PID 调参：1 kHz 控制环 + motor1 方波（此模式下别开 motor_task） */
  configASSERT(xTaskCreate(pid_tune_task, "pid_tune", 768, NULL, tskIDLE_PRIORITY + 6, NULL) == pdPASS);
#endif

#if APP_TEST_R9DS
  /* R9DS 自检：200 ms 打一行通道值（改 r9ds_test.cpp 的开关可切在线状态） */
  configASSERT(xTaskCreate(r9ds_test_task, "r9ds", 512, NULL, tskIDLE_PRIORITY + 4, NULL) == pdPASS);
#endif

#if APP_TEST_OMNI3
  /* 底盘运动学自测：2 s 一段，200 ms 打三轮目标/实测（速度环由 motor_task 跑） */
  configASSERT(xTaskCreate(omni3_test_task, "omni3", 512, NULL, tskIDLE_PRIORITY + 4, NULL) == pdPASS);
#endif


  /* 置运行标志：任务入口的 wait_running() 在此之后才放行 */
  configASSERT(sys_state.complete_init() == Status::OK);
}


/**
 * @brief 默认任务，这个原本命名为_start_default_task。但是每次开FreeRTOS这个里面，默认是这个名字
 *
 * @note 保留这个名字，但是其他任务要类似：_start_default_task
 * @param argument 任务参数
 */
extern "C" void StartDefaultTask(void *argument)
{
  for (;;)
  {

    vTaskDelay(100);
  }
}

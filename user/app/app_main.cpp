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
#include "cmsis_os2.h" // IWYU pragma: keep（osPriorityNormal / osPriorityHigh）

#include "board_cfg.hpp"    // 板级实例（bsp_uart3 打印、motor1/2/3）
#include "service/system/fault_record.hpp" // ICBSP_CHECK
#include "module_cfg.hpp"   // chassis
#include "service/system/system.hpp"       // system_init / system_start / sys_state

#include "app_task.hpp"
#include "app_test.hpp" // 测试开关（默认全 0：不创建任何自测任务）


/* ==================== */
/* ==================== sys_task 报告 ==================== */

/**
 * @brief sys_task 的周期报告：每 5 s 把维护耗时打到调试串口（库本身不打印，见 system_set_report_hook()）
 * @note 跑在 sys_task 的栈上；printf 的 vsnprintf 要栈，库任务栈 SYS_TASK_STACK 已按这个留了余量。
 */
static void sys_report(const SysTaskStats *stats)
{
  (void)print_uart("[sys] time=%lus cycle=%luus avg=%luus max=%luus\r\n",
                   static_cast<unsigned long>(xTaskGetTickCount() / configTICK_RATE_HZ),
                   static_cast<unsigned long>(stats->last_us),
                   static_cast<unsigned long>(stats->avg_us),
                   static_cast<unsigned long>(stats->max_us));
}

/* ==================== */
/* ==================== 入口 ==================== */

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
 *       所有带副作用的检查（init()、xTaskCreate()）一律用 ICBSP_CHECK：
 *       它总会求值，不会随断言被编译掉。任务优先级必须低于库任务
 *       （CAN 54 / BMI088 53 / sys 52，见 hardware_config.h）。
 *       自测任务由 `app_test.hpp` 的 `APP_TEST_*_ENABLED` 开关控制，默认全 0（不创建）。
 */
void app_init(void)
{
  /* 1. 库入口：sys_state + sys_task（10 ms）。调度器还没启动，这里只是创建 */
  system_init();

  /* 2. 板级：各外设 init()、设备 init()（每路 ICBSP_CHECK） */
  board_init();

  /* 3. 模块层：三全向轮底盘（依赖三个已绑定的 DeviceMotor） */
  module_init();

  /* sys_task 的维护耗时：库只计数（全局 sys_task_stats），要打印就在这里接回调；不要可以删掉这一行 */
  system_set_report_hook(sys_report, 5000U);

  /* 4. 放行：置运行位。业务任务入口的 wait_running() 在这之后才通过 */
  system_start();

  /* 5. 业务任务（入口第一句都是 sys_state.wait_running()） */
  /* 按键任务：200 ms 轮询（短按/长按做不同提示） */
  ICBSP_CHECK(xTaskCreate(key_task, "key", 256, NULL, osPriorityNormal, NULL) == pdPASS);

  /* 电机任务：1 kHz 速度环（周期在 motor_task.cpp 里改，dt 由 DWT 实测） */
  ICBSP_CHECK(xTaskCreate(motor_task, "motor", 512, NULL, osPriorityHigh, NULL) == pdPASS);

  /* 遥控任务：100 Hz，读 R9DS → 底盘目标（独占 r9ds.update()） */
  ICBSP_CHECK(xTaskCreate(remote_task, "remote", 512, NULL, osPriorityNormal, NULL) == pdPASS);

  /* 陀螺仪任务：1 kHz 读 UART2，把 Z 轴（az/wz/yaw）最近两帧存进 wt102（独占 update()） */
  ICBSP_CHECK(xTaskCreate(imu_task, "imu", 512, NULL, osPriorityHigh, NULL) == pdPASS);

  /* ==================== 自测任务（开关见 app_test.hpp，默认全 0） ==================== */

#if APP_TEST_MOTOR_PID_TUNE_ENABLED
  /* PID 调参：1 kHz 控制环 + motor3 方波（此模式下别开 motor_task） */
  ICBSP_CHECK(xTaskCreate(pid_tune_task, "pid_tune", 768, NULL, osPriorityHigh, NULL) == pdPASS);
#endif

#if APP_TEST_RC_R9DS_ENABLED
  /* R9DS 自检：200 ms 打一行通道值（改 r9ds_test.cpp 的开关可切在线状态） */
  ICBSP_CHECK(xTaskCreate(r9ds_test_task, "r9ds", 512, NULL, osPriorityNormal, NULL) == pdPASS);
#endif

#if APP_TEST_CHASSIS_OMNI3_ENABLED
  /* 底盘运动学自测：2 s 一段，200 ms 打三轮目标/实测（速度环由 motor_task 跑） */
  ICBSP_CHECK(xTaskCreate(omni3_test_task, "omni3", 512, NULL, osPriorityNormal, NULL) == pdPASS);
#endif
}


/**
 * @brief 默认任务体（CubeMX defaultTask，由 freertos.c 的 StartDefaultTask() 转调）
 * @param argument 任务参数
 * @note 不在 C++ 里直接定义 StartDefaultTask：CubeMX 默认把它生成成 __weak 以外的情况会重复定义。
 */
extern "C" void app_default_task(void *argument)
{
  (void)argument;
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(1000)); // 空循环，保留任务
  }
}

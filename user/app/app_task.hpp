/**
 * @file app_task.hpp
 * @author Rh
 * @brief 应用层任务声明 —— 除默认任务外的业务任务
 *
 * @note 任务函数均以 extern "C" 声明（FreeRTOS 以 C 方式调用）。
 *       实现位于 `user/app/task/`，由 `app_main.cpp` 的 `app_init()` 统一创建。
 *       任务入口第一句都是 `sys_state.wait_running()`：初始化失败时任务自行退出。
 *
 * @copyright Copyright (c) 2026
 */

#ifndef __APP_TASK_HPP__
#define __APP_TASK_HPP__

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief 按键任务：200 ms 轮询 key_user，按事件驱动板载 LED
   * @param argument 任务参数（未使用，NULL）
   * @note 轮询周期与 key_user 的消抖 / 长按配置对应（debounce 1 → 200 ms，
   *       long_press 5 → 1 s）。
   */
  void key_task(void *argument);

  /**
   * @brief 电机速度环任务：默认 1 ms（1 kHz）周期，跑 motor1/2/3 的 update()
   * @param argument 任务参数（未使用，NULL）
   * @note 控制周期改 `motor_task.cpp` 里的 MOTOR_TASK_PERIOD_MS；dt 由 bsp_dwt 实测，
   *       改频率后需要重新标定 PID（ki / kd 是每拍量纲）。
   */
  void motor_task(void *argument);

  /**
   * @brief 遥控任务：R9DS 通道 → 底盘速度（100 Hz）
   * @param argument 任务参数（未使用，NULL）
   * @note CH3 前后、CH4 平移、CH1 自旋；CH5 三挡：+100 自动（接口暂留）/ 0 停车 / -100 遥控。
   *       失联或非遥控挡一律刹车。本任务独占 r9ds.update()。
   */
  void remote_task(void *argument);

#ifdef __cplusplus
}
#endif

#endif // __APP_TASK_HPP__

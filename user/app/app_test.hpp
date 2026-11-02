/**
 * @file app_test.hpp
 * @author Rh
 * @brief 自测任务的函数声明（`user/app/test/` 下的实现）
 *
 * @note **默认全部不创建**：这些任务只改目标/只打印，会抢 PWM 或刷串口，
 *       需要哪一项就在 `app_main.cpp` 的 `app_init()` 里自己加一行 `xTaskCreate(...)`。
 *
 * @copyright Copyright (c) 2026
 */

#ifndef __APP_TEST_HPP__
#define __APP_TEST_HPP__

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief PID 调参任务：1 kHz 跑三路 update()，只给 motor1 方波 ±30 cm/s
   * @note 用它就不要同时创建 motor_task（两边抢同一批 PWM）。
   */
  void pid_tune_task(void *argument);

  /**
   * @brief R9DS 自检任务：打印 10 个通道语义值，或在线状态/诊断计数
   * @note 打印内容由 `r9ds_test.cpp` 里的 `R9DS_TEST_PRINT_CHANNELS` 选；
   *       与电机任务互不干扰，可以同时跑。
   */
  void r9ds_test_task(void *argument);

  /**
   * @brief 三全向轮底盘运动学自测：每 2 s 换一段，100 ms 打三轮目标/实测
   * @note 依赖 motor_task 在跑（它就是速度环）；本任务不碰 PWM 与编码器。
   */
  void omni3_test_task(void *argument);

#ifdef __cplusplus
}
#endif

#endif // __APP_TEST_HPP__

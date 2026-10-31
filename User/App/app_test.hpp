/**
 * @file app_test.hpp
 * @author Rh
 * @brief 测试开关（测完把对应开关改 0，正式代码里不留测试任务）
 * @version 0.1
 * @date 2026-10-29
 *
 * @copyright Copyright (c) 2026
 */

#ifndef __APP_TEST_HPP__
#define __APP_TEST_HPP__

/**
 * @brief 三电机开环正反转
 * @note 1 = 创建 motor_openloop_test_task：20 ms 一步，占空比 0→100→0 三角波；
 *       正转 = A 路出力、B 路给 0；反转 = B 路出力、A 路给 0。
 * @note 用这个就不开 motor_task（两边会抢同一批 PWM）。
 */
#define APP_TEST_MOTOR_OPENLOOP 0

/**
 * @brief PID 调参任务（方波阶跃）
 * @note 1 = 创建 pid_tune_task：1 kHz 跑三路 update()，但只给 motor1 玩方波 ±30 cm/s；
 *       每 10 ms 用 JustFloat 发一次三台的「目标/实测」。
 * @note 用这个就不要开 motor_task / 开环任务（三边抢 PWM）。
 */
#define APP_TEST_PID_TUNE 0

/**
 * @brief R9DS 接收机自检（UART4 / SBUS）
 * @note 1 = 创建 r9ds_test_task：默认 200 ms 打一行 10 个通道的语义值（-100~+100）；
 *       把 r9ds_test.cpp 里 R9DS_TEST_PRINT_CHANNELS 改 0 则改为 20 ms 打在线状态/诊断计数。
 * @note 与电机任务互不干扰，可以同时开。
 */
#define APP_TEST_R9DS 0
/**
 * @brief 三全向轮底盘运动学自测
 * @note 1 = 创建 omni3_test_task：每 2 s 切换一段（前进/后退/右移/左移/正反自转），
 *       200 ms 打印三轮「目标 / 实测」。
 * @note 依赖 motor_task 在跑（它就是速度环）；本任务不碰 PWM 与编码器。
 */
#define APP_TEST_OMNI3 0
#ifdef __cplusplus
extern "C"
{
#endif

  /** @brief 三电机开环正反转测试任务（20 ms 一步） */
  void motor_openloop_test_task(void *argument);

  /** @brief PID 调参任务：1 kHz 控制环 + 单电机方波给定 */
  void pid_tune_task(void *argument);

  /** @brief R9DS 自检任务（通道值 / 在线状态） */
  void r9ds_test_task(void *argument);

  /** @brief 三全向轮底盘运动学自测任务 */
  void omni3_test_task(void *argument);

#ifdef __cplusplus
}
#endif

#endif // __APP_TEST_HPP__

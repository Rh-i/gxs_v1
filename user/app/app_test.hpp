/**
 * @file app_test.hpp
 * @author Rh
 * @brief 测试开关（测完把对应开关改 0，正式代码里不留测试任务）
 * @version 0.2
 * @date 2026-10-31
 *
 * @copyright Copyright (c) 2026
 *
 * @note 命名统一 `APP_TEST_<域>_<功能>_ENABLED`；测试源文件按域放在 user/app/test/<域>/ 下。
 */

#ifndef __APP_TEST_HPP__
#define __APP_TEST_HPP__

/**
 * @brief 三电机开环正反转（user/app/test/motor/）
 * @note 1 = 创建 motor_openloop_test_task：20 ms 一步，占空比 0→100→0 三角波；
 *       正转 = A 路出力、B 路给 0；反转 = B 路出力、A 路给 0。
 * @note 用这个就不开 motor_task（两边会抢同一批 PWM）。
 * @warning 任务函数目前只有声明、没有实现文件，打开本宏会链接失败，需先补
 *          `user/app/test/motor/motor_openloop_test.cpp`。
 */
#define APP_TEST_MOTOR_OPENLOOP_ENABLED 0

/**
 * @brief PID 调参任务（方波阶跃，user/app/test/motor/pid_tune_test.cpp）
 * @note 1 = 创建 pid_tune_task：1 kHz 跑三路 update()，但只给 motor1 玩方波 ±30 cm/s；
 *       每 10 ms 用 JustFloat 发一次三台的「目标/实测」。
 * @note 用这个就不要开 motor_task / 开环任务（三边抢 PWM）。
 */
#define APP_TEST_MOTOR_PID_TUNE_ENABLED 0

/**
 * @brief R9DS 接收机自检（UART4 / SBUS，user/app/test/rc/r9ds_test.cpp）
 * @note 1 = 创建 r9ds_test_task：默认 200 ms 打一行 10 个通道的语义值（-100~+100）；
 *       把 r9ds_test.cpp 里 R9DS_TEST_PRINT_CHANNELS 改 0 则改为 20 ms 打在线状态/诊断计数。
 * @note 与电机任务互不干扰，可以同时开。
 */
#define APP_TEST_RC_R9DS_ENABLED 0

/**
 * @brief 三全向轮底盘运动学自测（user/app/test/chassis/omni3_test.cpp）
 * @note 1 = 创建 omni3_test_task：每 2 s 切换一段（前进/后退/右移/左移/正反自转），
 *       200 ms 打印三轮「目标 / 实测」。
 * @note 依赖 motor_task 在跑（它就是速度环）；本任务不碰 PWM 与编码器。
 */
#define APP_TEST_CHASSIS_OMNI3_ENABLED 0

#ifdef __cplusplus
extern "C"
{
#endif

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

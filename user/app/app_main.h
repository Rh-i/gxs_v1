/**
 * @file app_main.h
 * @author Rh
 * @brief 应用层与 STM32 的接口：初始化入口与任务声明
 *
 * @note 初始化入口只有 `app_init()`，由 freertos.c 的 `MX_FREERTOS_Init()` 调用；
 *       任务体在 `user/app/task/`，自测代码在 `user/app/test/`。
 *
 * @copyright Copyright (c) 2026
 */

#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__

#include "FreeRTOS.h" // IWYU pragma: keep
#include "semphr.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /* ==================== 初始化函数 ==================== */

  /**
   * @brief 库入口 → 板级初始化 → 模块校验 → 放行 → 创建业务任务
   *
   * @note 调度器启动前执行（MX_FREERTOS_Init 里），只创建任务与对象，不跑任务体。
   */
  void app_init(void);


  /* ==================== 任务函数声明 ==================== */

  /**
   * @brief 默认任务（CubeMX 生成，保留原名）
   * @param argument 任务参数
   */
  void StartDefaultTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif // __APP_MAIN_H__

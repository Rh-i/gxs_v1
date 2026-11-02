/**
 * @file board_instances.hpp
 * @brief 板级实例清单：库用 X-macro 按这里的名字展开，**没有**运行时登记表
 *
 * @details 每个名字必须已在 `board_cfg.hpp` 里定义；类型由 `decltype` 推出，所以改模板参数
 *          不用动这里。**漏写的表现是运行期收不到数据，不是编译失败**，加实例时两处一起改。
 *
 *          gxs_v1 没有 CAN、没有 SPI；UART 5 路全列如下。
 */

#ifndef __BOARD_INSTANCES_HPP__
#define __BOARD_INSTANCES_HPP__

#include "board_cfg.hpp" // IWYU pragma: keep

/** @brief CAN 实例列表：本工程没有 CAN，宏体写成空注释，展开后什么都不产生 */
#define BSP_CAN_INSTANCES(X) /* 无 */

/** @brief UART 实例列表：HAL 回调按句柄找实例，sys_task 每 10 ms 巡检收发健康度 */
#define BSP_UART_INSTANCES(X) \
  X(bsp_uart2)                \
  X(bsp_uart3)                \
  X(bsp_uart4)                \
  X(bsp_uart7)                \
  X(bsp_uart10)

/** @brief SPI 实例列表：本工程没有 SPI */
#define BSP_SPI_INSTANCES(X) /* 无 */

#endif // __BOARD_INSTANCES_HPP__

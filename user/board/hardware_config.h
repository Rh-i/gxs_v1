/**
 * @file hardware_config.h
 * @brief 本板的唯一配置入口：icbsp 的全部可调宏、功能开关与取值检查都集中在这里
 *
 * @details
 *
 * 【它是什么】
 *   icbsp 里**没有**任何“默认值头文件”。库的每一个可调参数都是本文件里的 `#define`，
 *   库源码经 `service/system/config_check.hpp` 包含它（宿主自己的代码可以直接包含本文件）。
 *   CMake 配置阶段会打印实际用的是哪一份：
 *
 *       -- icbsp: 使用配置 /…/user/board/hardware_config.h
 *
 *   所以“改库的行为”= 改本文件的宏，不用改库源码；换板 = 换一份本文件。
 *
 * 【四组宏怎么理解】
 *   ① 功能开关：决定“这段代码编不编译、这些任务建不建”——关掉省 Flash/RAM，但功能就没了
 *   ② 库任务：优先级与栈深——决定实时性和会不会栈溢出
 *   ③ 外设参数：队列深度、超时、缓冲区大小——决定吞吐、丢帧与阻塞时长
 *
 * 【检查在库里】
 *   缺宏、取值范围、开关之间的依赖都由库的 `code/service/system/config_check.hpp` 检查
 *   （编译期，不改行为）；CubeMX 没开对应 HAL 模块时库也会把开关强制置 0——本文件只需要写 `#define`。
 *
 * 【本板（gxs_v1）要点】
 *   - **没有 CAN、没有 SPI**：收发路径与 SPI 参数都关掉/保留默认即可
 *   - 5 路 UART 全 DMA：USART2（WT102）/ USART3（调试打印）/ UART4（R9DS SBUS）/
 *     UART7、USART10（备用）
 *   - 调试打印开关 `ICBSP_UART_PRINT_ENABLE` 与 `print_uart(...)` 在 `board_cfg.hpp`
 *     （要绑具体串口实例，跟着实例放）
 */

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include "FreeRTOS.h" // IWYU pragma: keep（configMAX_PRIORITIES）
#include "task.h"     // IWYU pragma: keep（configMAX_PRIORITIES）

/* ==================== ① 功能开关：关掉就不编译对应代码、也不创建对应任务 ==================== */

/** @brief 系统维护任务 sys_task（10 ms）：串口巡检、默认状态树总线层运行期记账 */
#define ICBSP_SYS_TASK_ENABLE 1
/** @brief CAN 接收路径（本板无 CAN） */
#define ICBSP_CAN_RX_ENABLE 0
/** @brief CAN 发送路径（本板无 CAN） */
#define ICBSP_CAN_TX_ENABLE 0
/** @brief sys_task 里的串口巡检：DMA 出错 / HAL 卡住时自动重建收发 */
#define ICBSP_UART_PATROL_ENABLE 1

/* ==================== */
/* ==================== 库任务（system_init() 创建） ==================== */

/** @brief 系统维护任务 sys_task 的优先级（configMAX_PRIORITIES - 4 = 52） */
#define ICBSP_SYS_TASK_PRIORITY (configMAX_PRIORITIES - 4)
/** @brief sys_task 栈深（字） */
#define ICBSP_SYS_TASK_STACK 256

/** @brief CAN 收 / 发任务的优先级（本板无 CAN，保留默认值即可） */
#define ICBSP_CAN_TASK_PRIORITY (configMAX_PRIORITIES - 2)
/** @brief CAN 收 / 发任务栈深（字） */
#define ICBSP_CAN_TASK_STACK 512

/* ==================== */
/* ==================== CAN ==================== */

/** @brief 每路 CAN 的回退缓冲深度（帧）；ICBSP_CAN_RX_ENABLE = 0 时不用 */
#define ICBSP_CAN_RX_RETURN_FRAMES 16
/** @brief can_rx_task 每 1 ms 每条总线最多分发的帧数（1~64） */
#define ICBSP_CAN_RX_DISPATCH_PER_TICK 16
/** @brief 接收节点数上限（1~255） */
#define ICBSP_CAN_RX_NODE_MAX 50
/** @brief 发送节点数上限（1~255） */
#define ICBSP_CAN_TX_NODE_MAX 50
/** @brief 控制指令有效期 (ms) */
#define ICBSP_CAN_CMD_EXPIRE_MS 100
/** @brief 保底心跳 (ms) */
#define ICBSP_CAN_TX_HEARTBEAT_MS 5
/** @brief 共帧槽位凑不齐时最多等多久就发 (ms) */
#define ICBSP_CAN_TX_GATHER_MS 1
/** @brief Bus-Off 恢复的重试间隔 (ms) */
#define ICBSP_CAN_BUSOFF_RETRY_MS 100

/* ==================== */
/* ==================== UART ==================== */

/**
 * @brief BspUartT::init() 打开 UART 硬件 FIFO（16 字节）；0 = 保持 CubeMX 的设置
 *
 * @note 本板（H7）有硬件 FIFO，但按新版默认取 0：收发都走 DMA / IDLE，不依赖 FIFO；
 *       开着只会让 tx_recover() 中止发送时多清掉最多 16 字节。
 */
#define ICBSP_UART_ENABLE_FIFO 0

/* ==================== */
/* ==================== SPI ==================== */

/** @brief 每个 BspSpi 实例的 DMA 弹跳缓冲（字节，收发各一份） */
#define ICBSP_SPI_DMA_BUFFER_SIZE 256
/** @brief BspSpi / BspSpiDevice 传输接口的默认超时 (ms) */
#define ICBSP_SPI_TRANSFER_TIMEOUT_MS 10

/* ==================== */
/* ==================== BMI088（本板未使用，保留默认值） ==================== */

/** @brief DeviceBmi088 自动刷新任务的优先级 */
#define ICBSP_BMI088_TASK_PRIORITY (configMAX_PRIORITIES - 3)
/** @brief DeviceBmi088 自动刷新任务栈深（字） */
#define ICBSP_BMI088_TASK_STACK 512

/* ==================== */

#endif /* HARDWARE_CONFIG_H */

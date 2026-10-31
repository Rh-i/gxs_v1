# Module 层

> 依赖：[分层架构](../../Docs/spec/分层架构.md)　编码：[编码规范](../../Docs/spec/编码规范.md)

Module 把**多个 Device** 组合成一个机构：只做机构级的坐标变换与协调，不下沉到单设备细节。

## 模块文档

| 模块 | 文档 | 内容 |
| --- | --- | --- |
| `module_cfg` | 本文件 | 全局实例、`module_init()` |
| `omni3_chassis` | [omni3_chassis.md](omni3_chassis.md) | 三轮全向运动学、符号约定、系数表、测试判据 |

## 层规则

- 可放：机构运动学、机构状态、由多台设备组成的协同控制。
- 不可放：协议编解码、设备寄存器、PWM/编码器细节、任务创建。
- 只依赖 `Device` 及以下；不得 `#include "app_*"`。

## 全局实例

`module_cfg.{hpp,cpp}`：实例定义在 `.cpp`、`extern` 在 `.hpp`，`module_init()` 由 `all_init()`
在 `device_init()` 之后调用。

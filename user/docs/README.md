# 开发文档索引

| 目录 | 用途 | 文档 |
| --- | --- | --- |
| `guide/` | 理解工程结构、搭环境、编译烧写调试 | [项目结构](guide/项目结构.md)、[开发环境与烧录调试](guide/开发环境与烧录调试.md)、[clangd 配置](guide/clangd配置.md) |
| `spec/` | 写代码必须遵守的规范与分层原则 | [编码规范](spec/编码规范.md)、[分层架构](spec/分层架构.md) |
| `build/` | 构建脚本与链接脚本的改动记录 | [构建系统改动](build/构建系统改动.md)、[链接脚本改动](build/链接脚本改动.md) |
| `hardware/` | 引脚分配与复用冲突 | [引脚分配与冲突](hardware/引脚分配与冲突.md) |
| `plan/` | 进度、待办、方案与迁移记录 | [进度与待办](plan/进度与待办.md)、[迁移合并记录](plan/迁移合并记录.md) |
| `omni3/` | 三轮布局图与生成脚本 | `omni3_layout.py` / `.svg` / `.png` |
| `sheet/` | 器件手册、开发板说明书与管脚标注图（PDF / docx） | — |
| `image/` | 文档插图 | — |

> 业务库（bsp / service / device / protocol / algorithm / tasks）已在
> `Middlewares/Third_Party/stm32-framework`（git submodule），其文档在该库的 `docs/`：
> `接入指南.md`、`常用接口.md`、`编码规范.md`、`code/*.md`。
>
> 本工程只保留 `user/{app,board,module}`。`plan/CoreXY*` 是旧板搬入的留档方案，
> 本板机构是三轮全向（`user/module/omni3_chassis`）。

## 写给 AI 的话

每次生成内容前，先阅读本目录下的文档，理解工程组成并遵守其中的要求；修改文档内容时，
务必提醒开发者阅读并润色。

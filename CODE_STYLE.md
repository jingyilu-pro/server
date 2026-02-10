# 项目代码风格记录（现状）

本文档基于当前仓库代码实测整理，目标是**记录现有风格**，不是引入新规范。

## 适用范围

- 一方代码：`common/`、`app/`、根目录 `CMakeLists.txt`、`cmake/`。
- 不纳入风格基线：`libs/` 下第三方子模块、`app/protocol/protocol/base.pb.*`（生成文件）、`build-wsl*/`（构建产物）。

## 语言与构建基线

- C++ 标准：`C++20`。
- CMake 最低版本：`3.28`。
- 编译参数以 `CMakeLists.txt` 中现有设置为准（`-O2 -Wall -fcoroutines -fpermissive` 等）。

## 文件头与注释

- 大多数一方 `.h/.cpp` 文件顶部包含统一版权/许可注释块。
- 注释语言为中英混合，常见行注释使用 `//`。
- 保留历史注释和调试注释（例如被注释掉的日志/输出）是当前代码常态。

## 头文件与包含习惯

- 头文件防护使用 `#pragma once`。
- `#include` 顺序在项目内并不完全统一（存在“先本地后系统”与“先系统后本地”两种）。
- 常见做法是使用相对短路径包含项目头（例如 `"define.h"`、`"service.h"`）。

## 命名风格

- 类名：`PascalCase`（如 `Application`、`MaskWordService`、`CoroManager`）。
- 成员变量：`m_` 前缀 + 小写下划线（如 `m_services`、`m_worker_threads`）。
- 方法/函数：以小写或小写下划线为主（如 `start`、`set_log_level`、`await_suspend_handle`）。
- 宏：全大写下划线（如 `SAFE_DELETE`、`PROPERTY`、`BITMASK_CHECK_ANY`）。
- 常量：`constexpr` + 小写下划线（如 `coro_result_recycle_interval`、`block_size`）。
- 类型别名：保留传统 `typedef` 写法（如 `uint64`、`int32`）。

## 缩进、空白与大括号

- 大括号风格整体偏 `Allman`（控制语句和函数体常见换行开括号）。
- 缩进以 4 空格为主，但部分文件/代码块存在 Tab 缩进（混用现象）。
- 条件语句常见写法：`if(...)`（括号前不额外空格）；`for (...)`（关键字后保留空格）也有出现。
- 空行使用较宽松，函数间通常留空行分隔。

## 面向对象与内存管理风格

- 明确使用 `virtual`/`override` 的继承接口风格。
- 仍大量使用原始指针与手动 `new/delete`。
- 项目中存在宏辅助资源释放（如 `SAFE_DELETE`）。
- 对象池/协程结果对象采用复用模式，生命周期由业务管理器控制。

## 命名空间与标准库使用

- 多个头文件/源文件使用 `using namespace std;`（包括头文件）。
- 代码中同时使用 `std::` 显式限定与非限定调用两种方式。

## 并发与协程相关写法

- 协程封装使用自定义 `coro_task` / `coro_awaitable` / `CoroResult` 抽象。
- 线程模型使用 `std::thread` + 队列（`moodycamel`）组合。
- 并发代码中保留简洁的早返回风格（如 `if(co == 0) continue;`）。

## CMake 风格

- CMake 命令大小写不完全统一，存在大量全大写调用（如 `Project`、`SET`、`MESSAGE`）。
- 子模块构建大量采用 `ExternalProject_Add`。
- 源文件收集习惯使用 `file(GLOB_RECURSE ...)`。
- 目标链接使用集中变量（如 `DEPENDENCIES_LIBS`）统一聚合。

## 新代码落地建议（与现状保持一致）

- 优先遵循“就近一致”：在某文件中延续该文件既有缩进和空白风格。
- 新增类与成员命名保持 `PascalCase + m_` 约定。
- 新增头源文件延续当前版权头模板。
- 不在单次改动中大规模重排 include 或统一格式，避免引入无关 diff。
- 如需后续“规范化重构”（例如去除头文件内 `using namespace std;`），建议单独开 PR 做机械化改动。


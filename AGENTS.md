# IM Chat Project Development Rules

## 1. Project Overview

本项目是一个类似 QQ 的桌面即时通信系统，包含：

- Qt 桌面客户端
- Linux Ubuntu 服务端
- 用户注册和登录
- 好友管理
- 单聊和群聊
- 离线消息
- 文件传输
- 数据库存储和服务端部署

项目目标是形成一个可运行、可测试、可部署、适合写入简历的 C++ IM 项目。

## 2. Technology Stack

### Client

- Qt 6
- C++17
- Qt Widgets
- CMake
- SQLite
- Qt Network
- Qt Test

### Server

- C++17
- Boost.Asio
- CMake
- MySQL
- Redis 可作为后续扩展
- GoogleTest
- Ubuntu Linux
- Docker Compose

### Engineering

- Git
- CMake
- Docker
- GitHub Actions 可作为后续扩展

## 3. Initial Architecture

建议使用模块化单体架构，不要一开始拆分成微服务。

客户端模块：

- `client/ui/`：登录、注册、主窗口、聊天窗口
- `client/network/`：连接管理、协议编解码、重连
- `client/models/`：好友、群组、消息模型
- `client/services/`：认证、好友、消息、文件服务
- `client/storage/`：SQLite 数据访问
- `client/utils/`：通用工具

服务端模块：

- `server/gateway/`：网络连接和会话管理
- `server/auth/`：注册、登录、Token
- `server/user/`：用户信息
- `server/friend/`：好友关系
- `server/group/`：群组和成员
- `server/message/`：单聊、群聊、离线消息
- `server/file/`：文件元数据和传输控制
- `server/database/`：数据库访问
- `server/protocol/`：请求、响应和消息协议
- `server/tests/`：单元测试和集成测试

如果实际目录与以上结构不一致，应优先保持已有代码的一致性，不要为了目录名称进行无意义重构。

## 4. Coding Rules

- 使用 C++17。
- 类名使用 `PascalCase`。
- 函数和变量使用 `camelCase`。
- 常量使用 `kPascalCase` 或项目已有统一风格。
- 优先使用 RAII、智能指针和标准容器。
- 避免全局可变状态。
- 头文件使用 include guard 或 `#pragma once`。
- 新增模块必须说明职责，避免超大类。
- 不要修改与当前功能无关的代码。
- 不要提交编译产物、临时文件、日志和本地配置文件。

## 5. Network Rules

- 所有请求必须包含 `request_id`。
- 所有响应必须包含 `request_id`、`code` 和 `message`。
- 网络读取必须正确处理半包、粘包和拆包。
- 不假设一次 `read` 就能读完整消息。
- 必须处理客户端断线、服务端异常和超时。
- 登录成功后使用 Token 或 Session 标识用户。
- 重要操作必须校验用户身份和权限。
- 网络协议的新增或修改必须同步更新协议文档和测试。

## 6. Database Rules

- 所有 SQL 必须使用参数化查询或预编译语句。
- 禁止明文保存密码。
- 密码必须使用 Argon2、bcrypt 或同等级别算法哈希。
- 数据库表结构变更必须记录在 migration 文件中。
- 用户、好友、群组和消息关系应使用独立数据表保存。
- 不要把好友列表或群成员列表直接保存成不可查询的字符串。
- 为高频查询字段建立合理索引。
- 数据库连接和事务必须正确释放。

## 7. Security Rules

- 禁止把数据库密码、Token 密钥和私钥提交到 Git。
- 敏感配置使用环境变量或本地配置文件。
- 文件上传必须限制大小、文件名和存储路径。
- 文件下载必须进行权限检查。
- 服务端不能直接信任客户端传入的用户 ID。
- 所有用户输入都必须进行长度、格式和权限校验。
- 日志不能输出密码、完整 Token 或其他敏感信息。

## 8. Build and Test

构建目录统一使用：

```text
build/
```

构建命令示例：

```bash
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build --output-on-failure
```

每个新功能至少应包含：

- 正常流程测试
- 参数非法测试
- 未登录或无权限测试
- 断线或异常流程测试

在报告任务完成前，必须尽量执行编译和相关测试，并说明未执行的检查。

## 9. Development Workflow

每次开发遵循以下流程：

1. 先读取当前目录、相关代码和已有测试。
2. 说明准备修改的范围。
3. 将功能拆成尽可能小的实现步骤。
4. 只修改当前功能需要的文件。
5. 编译并运行相关测试。
6. 检查安全性、异常处理和资源释放。
7. 总结修改文件、运行方式和已知风险。

推荐提交信息格式：

```text
feat: add user registration
feat: add login session
feat: add friend request
feat: add offline message
fix: handle partial tcp packet
test: add group message tests
docs: update deployment guide
```

## 10. Codex Instructions

使用 Codex 时：

- 不要一次要求实现完整 IM 系统。
- 每次只实现一个可验证的功能闭环。
- 修改前必须先检查已有代码。
- 不要猜测文件内容、接口或运行结果。
- 遇到错误时优先分析日志和复现步骤。
- 完成后必须列出修改文件和验证命令。
- 未经明确要求，不要删除已有代码或重写整个模块。
- 设计新接口时，要考虑后续单聊、群聊和文件传输的扩展性。

## 11. Definition of Done

一个功能只有同时满足以下条件，才算完成：

- 代码可以编译。
- 正常流程可以运行。
- 主要异常流程有处理。
- 相关测试通过。
- 没有提交敏感信息。
- 协议、数据库或部署文档已同步更新。
- 能够说明如何启动和验证。

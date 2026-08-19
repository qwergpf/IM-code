# 第三阶段服务端代码学习指南

> 本文完全基于当前项目中已经实现的代码，目标是帮助学习者理解 C++、Linux、TCP、MySQL、Protobuf 和工程化知识，并能够在面试中准确介绍项目。
>
> 当前阶段实现的是服务端基础框架，不是完整 IM 系统。登录、注册、好友、聊天、文件传输、正式心跳等业务尚未实现。

## 1. 当前阶段实现范围

当前服务端已经实现：

- Ubuntu 下的 C++17 服务端程序入口。
- 配置文件和环境变量加载。
- MySQL 连接和 `SELECT 1` 健康检查。
- Boost.Asio 异步 TCP 监听和客户端连接管理。
- 8 字节自定义网络包头。
- Protobuf 序列化与反序列化。
- TCP 半包与粘包处理。
- Ping 请求。
- 数据库健康检查请求。
- 登录和注册协议占位。
- 基础日志、单元测试和集成测试。

尚未实现：

- 用户表和数据库 migration。
- 用户注册和登录业务。
- 密码哈希。
- Session 或 Token。
- 好友、群组和聊天业务。
- 在线状态和正式心跳。
- 数据库工作线程和连接池。
- TLS 加密。
- 优雅退出、限流和生产级监控。

## 2. 服务端整体架构

当前服务端采用：

```text
单进程
  + 单 io_context 线程
  + Boost.Asio 异步 TCP
  + 单个 MySQL 长连接
  + 8 字节自定义包头
  + Protobuf 消息体
  + 模块化单体结构
```

实际调用关系：

```text
main.cpp
    │
    ├── ConfigManager
    │      ├── 读取 server.conf
    │      └── 读取 IM_DB_PASSWORD
    │
    ▼
ServerApplication
    │
    ├── DatabaseManager
    │      ├── mysql_init
    │      ├── mysql_real_connect
    │      └── SELECT 1
    │
    ├── ProtocolHandler
    │      ├── Ping
    │      ├── DatabaseHealth
    │      └── Login/Register 返回 NOT_IMPLEMENTED
    │
    ├── boost::asio::io_context
    │
    └── NetworkServer
           │
           ├── tcp::acceptor
           ├── async_accept
           │
           ▼
       ClientConnection
           │
           ├── async_read_some
           ├── PacketStreamDecoder
           ├── ProtocolHandler
           ├── PacketCodec
           └── async_write
```

按分层理解：

```text
Qt 客户端（网络功能尚未实现）
        │
        │ TCP
        ▼
NetworkServer / ClientConnection
        │
        ▼
PacketCodec / PacketStreamDecoder
        │
        ▼
ProtocolHandler
        │
        ▼
DatabaseManager
        │
        ▼
MySQL
```

### 2.1 模块职责

| 模块 | 实际类或文件 | 职责 |
| --- | --- | --- |
| 程序入口 | `main.cpp` | 参数解析和顶层异常处理 |
| 服务组装 | `ServerApplication` | 按顺序创建数据库、协议和网络模块 |
| 配置 | `ConfigManager` | 读取配置文件和环境变量 |
| 日志 | `Logger` | 统一输出 INFO、WARN、ERROR |
| 数据库 | `DatabaseManager` | 管理 MySQL 连接和健康检查 |
| TCP 监听 | `NetworkServer` | open、bind、listen、accept |
| 客户端会话 | `ClientConnection` | 管理一个客户端的读取、响应和断开 |
| 网络封包 | `PacketCodec` | 构造包头、序列化和反序列化 |
| 流式拆包 | `PacketStreamDecoder` | 解决半包和粘包 |
| 协议分发 | `ProtocolHandler` | 校验并处理不同消息类型 |
| 协议定义 | `im_protocol.proto` | 定义客户端和服务端共享的数据格式 |
| 构建系统 | CMake | 查找依赖、生成 Protobuf、编译和链接 |
| 测试 | GoogleTest、Python | 验证配置、协议、拆包和真实 TCP 连接 |

## 3. 程序启动流程

启动命令：

```bash
export IM_DB_PASSWORD='本地开发密码'
./build/server --config ./build/server.conf
```

完整执行流程：

```text
Linux 创建 server 进程
    ↓
进入 main()
    ↓
解析 --config 参数
    ↓
ConfigManager::load()
    ↓
读取 server.conf 和 IM_DB_PASSWORD
    ↓
ServerApplication::run()
    ↓
DatabaseManager::connect()
    ↓
DatabaseManager::verifyConnection()
    ↓
创建 ProtocolHandler
    ↓
创建 boost::asio::io_context
    ↓
创建 NetworkServer
    ↓
open → bind → listen → async_accept
    ↓
io_context.run()
    ↓
等待连接、读写和断开事件
```

任意启动步骤失败都会抛出异常，最终由 `main()` 记录错误并返回非零退出码。数据库连接失败时，服务端不会继续监听端口，这体现了 fail-fast 思想。

## 4. 为什么代码不能全部放进 main.cpp

如果配置、MySQL、TCP、拆包和业务全放在 `main.cpp` 中，会导致：

- 文件越来越大，难以阅读。
- 无法单独测试拆包逻辑。
- 修改数据库代码可能影响网络代码。
- 错误很难定位属于哪个模块。
- 多人无法方便地并行开发。
- 资源生命周期难以管理。

当前划分体现了高内聚：

- `PacketCodec` 只负责数据包。
- `DatabaseManager` 只负责数据库连接。
- `NetworkServer` 只负责监听和接收连接。
- `ClientConnection` 只负责一个客户端。
- `ProtocolHandler` 只负责协议校验和请求分发。

当前划分也体现了低耦合：

- 网络监听器不需要知道 MySQL 的连接方式。
- 数据包编解码器不需要知道登录 SQL。
- 数据库模块不依赖 Boost.Asio。
- 配置模块不关心配置最终由谁使用。

## 5. main.cpp

文件：`server/src/main.cpp`

### 5.1 文件职责

- 提供程序入口。
- 解析 `--config` 参数。
- 加载配置。
- 启动 `ServerApplication`。
- 捕获没有在内部处理的启动异常。

### 5.2 主要依赖

```cpp
#include "config/ConfigManager.h"
#include "logging/Logger.h"
#include "server/ServerApplication.h"
```

### 5.3 核心代码

```cpp
try {
    const ServerConfig config = ConfigManager::load(configPath);
    return ServerApplication{}.run(config);
} catch (const std::exception& error) {
    Logger::error(std::string("Server startup failed: ") + error.what());
    return 1;
}
```

【代码作用】

加载配置并启动服务端，所有启动异常最终在这里转换成日志和进程退出码。

【执行流程】

```text
配置加载成功 → 启动服务端
配置或启动失败 → catch → ERROR 日志 → 返回 1
参数错误 → 返回 2
正常结束 → 返回 0
```

【涉及的 C++ 知识】

- `try/catch` 和异常传播。
- `const` 对象。
- 临时对象 `ServerApplication{}`。
- `std::exception` 标准异常基类。

【涉及的 Linux 知识】

Linux Shell 可以查看退出码：

```bash
./build/server --config ./build/server.conf
echo $?
```

【为什么这样设计】

`main()` 只充当最外层错误边界和程序入口，不包含数据库或网络实现，有利于保持职责清晰。

## 6. ServerApplication

文件：

- `server/src/server/ServerApplication.h`
- `server/src/server/ServerApplication.cpp`

### 6.1 类职责

`ServerApplication` 是对象组装入口，负责决定初始化顺序，但不实现具体的数据库和网络细节。

核心函数：

```cpp
int ServerApplication::run(const ServerConfig& config);
```

使用 `const ServerConfig&` 的原因：

- 引用避免复制整个配置对象。
- `const` 保证启动过程不会修改外部配置。
- 配置含多个字符串和数据库密码，应减少不必要的复制。

### 6.2 数据库初始化

```cpp
auto database = std::make_shared<DatabaseManager>(config);
database->connect();
database->verifyConnection();
```

【代码作用】

创建数据库管理器、连接 MySQL，并执行健康检查。

【为什么先连接数据库】

如果先监听端口再连接数据库，客户端可能已经连接，但服务端无法处理需要数据库的请求，形成“端口可用、业务不可用”的假启动状态。

当前代码采用：

```text
配置成功
  ↓
数据库成功
  ↓
网络监听成功
```

### 6.3 网络初始化

```cpp
auto handler = std::make_shared<ProtocolHandler>(database);
boost::asio::io_context ioContext;
NetworkServer server(ioContext, config.listenIp, config.listenPort, handler);
ioContext.run();
```

【涉及的 C++ 知识】

- `std::shared_ptr` 共享所有权。
- 构造函数依赖注入。
- 栈对象和自动析构。
- RAII 生命周期管理。

【涉及的网络知识】

`io_context` 是 Boost.Asio 的事件调度中心，负责分发连接、读取、写入和错误事件。

Ubuntu 上 Boost.Asio 通常使用 epoll 作为底层 reactor，但当前代码没有直接调用 epoll API。

## 7. ConfigManager

文件：

- `server/src/config/ConfigManager.h`
- `server/src/config/ConfigManager.cpp`

### 7.1 ServerConfig

```cpp
struct ServerConfig
{
    std::string listenIp;
    std::uint16_t listenPort;
    std::string databaseHost;
    std::uint16_t databasePort;
    std::string databaseUsername;
    std::string databasePassword;
    std::string databaseName;
};
```

这是一个只保存配置数据的结构体。

端口使用 `std::uint16_t`，因为 TCP 端口最大为 65535。代码仍需额外拒绝端口 0。

### 7.2 load()

```cpp
static ServerConfig load(const std::string& path);
```

【代码作用】

- 使用 `std::ifstream` 打开配置文件。
- 按行读取 `key=value`。
- 忽略空行和 `#` 注释。
- 使用 `std::unordered_map` 保存解析结果。
- 验证必填配置。
- 从环境变量读取数据库密码。

【涉及的 C++ 知识】

- 文件流与 RAII。
- `std::getline`。
- `std::unordered_map` 哈希表。
- 字符串查找与截取。
- 异常处理。
- 匿名命名空间。

### 7.3 为什么密码使用环境变量

配置文件可以进入 Git，但真实密码不能提交。当前密码通过：

```cpp
std::getenv("IM_DB_PASSWORD");
```

读取。

这比把密码硬编码到 C++ 或提交到 Git 更安全。但环境变量不是绝对安全，密码仍可能出现在 Shell 历史或进程环境中。

### 7.4 parsePort()

```cpp
port = std::stoul(value, &parsedCharacters);
```

除了把字符串转成数字，还检查：

```cpp
parsedCharacters == value.size()
```

这样可以拒绝 `8888abc` 之类的非法输入，而不是错误地只解析出前面的 `8888`。

## 8. Logger

文件：

- `server/src/logging/Logger.h`
- `server/src/logging/Logger.cpp`

提供：

```cpp
Logger::info(...);
Logger::warn(...);
Logger::error(...);
```

统一输出格式：

```text
[2026-08-19 12:38:02][INFO] Server starting
```

### 8.1 为什么不在各处直接使用 std::cout

统一封装后，未来可以集中增加：

- 文件输出。
- 日志等级过滤。
- 日志轮转。
- 线程 ID。
- 连接 ID。
- request_id。

### 8.2 mutex 和 lock_guard

```cpp
std::lock_guard<std::mutex> lock(logMutex());
```

`lock_guard` 构造时加锁，离开作用域自动解锁，是 RAII 在互斥锁上的应用。

当前服务端只有一个 IO 线程，锁的作用不明显，但它可以避免未来多线程日志交叉输出。

### 8.3 当前不足

- 日志只输出到终端。
- 没有文件和轮转。
- 没有等级配置。
- 没有连接 ID 和 request_id 上下文。
- 尚未形成生产级日志系统。

## 9. DatabaseManager

文件：

- `server/src/database/DatabaseManager.h`
- `server/src/database/DatabaseManager.cpp`

### 9.1 MySQL 连接对象

MySQL C API 使用：

```cpp
MYSQL*
```

它表示一个 MySQL 客户端连接上下文，包含连接状态、认证状态、当前数据库和错误信息等。

原始 C API 需要手动调用：

```cpp
mysql_close(connection);
```

### 9.2 使用 RAII 管理 MYSQL*

```cpp
struct MysqlDeleter
{
    void operator()(MYSQL* connection) const noexcept;
};

std::unique_ptr<MYSQL, MysqlDeleter> connection_;
```

普通 `unique_ptr` 默认调用 `delete`，但 `MYSQL*` 不能通过 `delete` 释放，必须调用 `mysql_close()`。

因此代码为智能指针提供自定义删除器：

```cpp
void DatabaseManager::MysqlDeleter::operator()(MYSQL* connection) const noexcept
{
    if (connection != nullptr) {
        mysql_close(connection);
    }
}
```

这体现了 RAII：把资源释放绑定到 C++ 对象析构。

### 9.3 connect()

```cpp
MYSQL* rawConnection = mysql_init(nullptr);
std::unique_ptr<MYSQL, MysqlDeleter> connection(rawConnection);
```

先将连接放入局部智能指针。如果认证或字符集设置失败，局部对象离开作用域时会自动关闭连接。

只有所有步骤成功后才执行：

```cpp
connection_ = std::move(connection);
```

这相当于“初始化全部成功后再提交到成员状态”。

连接流程：

```text
mysql_init
  ↓
设置 5 秒连接超时
  ↓
mysql_real_connect
  ↓
MySQL TCP 握手和用户认证
  ↓
选择 im_chat 数据库
  ↓
设置 utf8mb4
```

### 9.4 为什么使用 utf8mb4

聊天系统需要支持中文、emoji、多语言昵称和消息。MySQL 的传统 `utf8` 只支持最多 3 字节字符，而 `utf8mb4` 支持完整 UTF-8。

### 9.5 verifyConnection()

```cpp
mysql_query(connection_.get(), "SELECT 1");
```

`SELECT 1` 不访问业务表，只验证：

- 连接存在。
- MySQL 可以处理 SQL。
- 客户端可以收到查询结果。

结果集 `MYSQL_RES*` 也使用自定义删除器，通过 `mysql_free_result()` 自动释放。

### 9.6 为什么不能每次查询都重新连接

每次建立 MySQL 连接都可能包含：

- TCP 握手。
- MySQL 协议握手。
- 用户认证。
- 初始化会话。

这些开销可能比简单 SQL 本身更大，因此应复用连接或使用连接池。

### 9.7 当前有没有连接池

没有。

当前只有：

```cpp
std::unique_ptr<MYSQL, MysqlDeleter> connection_;
```

也就是一个 MySQL 连接。

数据库健康检查还会在唯一的 Asio IO 线程同步执行。如果 SQL 耗时 3 秒，这 3 秒内所有客户端网络事件都无法处理。

### 9.8 未来扩展方向

推荐逐步形成：

```text
DatabaseManager
    ├── UserRepository
    ├── FriendRepository
    ├── GroupRepository
    └── MessageRepository
```

`DatabaseManager` 负责连接，Repository 负责具体业务 SQL。未来还需要数据库工作线程和连接池。

## 10. NetworkServer

文件：

- `server/src/network/NetworkServer.h`
- `server/src/network/NetworkServer.cpp`

### 10.1 tcp::acceptor

```cpp
boost::asio::ip::tcp::acceptor acceptor_;
```

它是监听 Socket 的 C++ 封装，对应传统 Socket 流程：

```text
socket()
  ↓
bind()
  ↓
listen()
  ↓
accept()
```

### 10.2 endpoint

```cpp
boost::asio::ip::tcp::endpoint endpoint(address, listenPort);
```

endpoint 表示 IP 地址与端口，例如：

```text
0.0.0.0:8888
```

`0.0.0.0` 表示监听本机全部网络接口。

### 10.3 reuse_address

```cpp
acceptor_.set_option(
    boost::asio::ip::tcp::acceptor::reuse_address(true)
);
```

对应 `SO_REUSEADDR`，有利于服务端重启后重新绑定端口。

### 10.4 async_accept()

```cpp
acceptor_.async_accept(
    [this](const boost::system::error_code& error,
           boost::asio::ip::tcp::socket socket) {
        ...
    }
);
```

【代码作用】

注册“有客户端连接时执行的回调”，不会阻塞当前线程等待客户端。

【涉及的 C++ 知识】

- Lambda。
- 捕获 `this`。
- 移动 Socket 所有权。
- 智能指针。

【涉及的 Linux 知识】

监听 Socket 在 Linux 中对应文件描述符。Asio 在 Ubuntu 上通常通过 epoll 等待连接事件。

一次 `async_accept` 只接收一个连接，因此回调末尾再次调用 `acceptNext()`，才能继续接收后续客户端。

## 11. ClientConnection

文件：

- `server/src/network/ClientConnection.h`
- `server/src/network/ClientConnection.cpp`

一个 `ClientConnection` 对象对应一个客户端 TCP 连接。

```text
客户端 A → ClientConnection A
客户端 B → ClientConnection B
客户端 C → ClientConnection C
```

每个对象拥有独立的：

- TCP Socket。
- 临时读取缓冲区。
- 流式拆包缓冲区。
- 异步写队列。
- 关闭状态。

### 11.1 enable_shared_from_this

```cpp
class ClientConnection final
    : public std::enable_shared_from_this<ClientConnection>
```

异步操作通常在函数返回之后才完成。如果对象提前销毁，回调访问对象就会产生悬空指针。

当前代码在发起异步操作前执行：

```cpp
auto self = shared_from_this();
```

Lambda 捕获 `self`：

```cpp
[self](...) { ... }
```

只要回调还未结束，这个 `shared_ptr` 就保证连接对象仍然存在。

### 11.2 readSome()

```cpp
socket_.async_read_some(
    boost::asio::buffer(readBuffer_),
    callback
);
```

`async_read_some` 只保证读取当前可用的一部分字节，不保证：

- 得到一个完整数据包。
- 只得到一个数据包。
- 与客户端的一次 `send` 一一对应。

一次读取可能得到半个包、一个包、一个半包或多个包，所以收到数据后必须交给 `PacketStreamDecoder`。

### 11.3 processPackets()

```cpp
while (!closing_) {
    const auto result = decoder_.next(packet);
}
```

使用循环是为了处理粘包。一次读取如果包含三个完整包，程序需要连续解析三次。

### 11.4 异步写队列

```cpp
std::deque<std::vector<std::uint8_t>> writeQueue_;
```

同一个 Socket 不应该同时重叠执行多个 `async_write`。当前流程：

```text
响应加入队列
  ↓
没有正在进行的写操作
  ↓
writeNext()
  ↓
写完队首
  ↓
删除队首
  ↓
继续写下一个
```

这样能够保证响应顺序和发送缓冲区生命周期。

## 12. Protobuf 协议定义

文件：`server/protocol/im_protocol.proto`

### 12.1 MessageType

消息类型示例：

```text
1   PingRequest
2   PingResponse
3   DatabaseHealthRequest
4   DatabaseHealthResponse
10  RegisterRequest
12  LoginRequest
100 ErrorResponse
```

### 12.2 Envelope

```proto
message Envelope {
  uint32 protocol_version = 1;
  string request_id = 2;

  oneof payload {
    PingRequest ping_request = 10;
    LoginRequest login_request = 22;
    ErrorResponse error_response = 100;
  }
}
```

`protocol_version` 用于协议兼容，当前固定为 1。

`request_id` 是应用层请求标识，客户端可以利用它把异步响应与原请求对应起来。它不是 TCP 序号，也不是数据库 ID。

`oneof` 表示一次 Envelope 只能选择一种 payload。

### 12.3 当前协议占位

协议中已有 `LoginRequest` 和 `RegisterRequest`，但服务端没有实现登录注册业务。收到这两类消息会返回：

```text
1006 NOT_IMPLEMENTED
```

协议里包含 password 字段，但当前没有 TLS，不能用于安全传输真实密码。

## 13. 自定义 TCP 数据包

TCP 只提供可靠字节流，不知道一个业务消息在哪里结束。因此项目在 TCP 上设计了应用层协议：

```text
┌────────────────────────┐
│ body_length    4 字节  │
├────────────────────────┤
│ message_type   4 字节  │
├────────────────────────┤
│ Protobuf body           │
└────────────────────────┘
```

总长度：

```text
8 + body_length
```

两个整数都使用网络字节序，也就是大端字节序。

例如 `message_type=1`：

```text
00 00 00 01
```

### 13.1 为什么需要 body_length

TCP 没有消息边界。客户端连续发送消息 A 和 B，服务端可能收到：

```text
A 的一半
```

也可能收到：

```text
完整 A + B 的一部分
```

长度字段让接收端知道应等待多少包体字节。

### 13.2 为什么需要 message_type

消息类型用于判断当前是 Ping、登录、注册还是其他请求。服务端还会验证包头类型是否与 Protobuf payload 一致。

例如包头写 Login，但 Protobuf 内是 Ping，服务端返回：

```text
1005 MESSAGE_TYPE_MISMATCH
```

## 14. PacketCodec

文件：

- `server/src/protocol/PacketCodec.h`
- `server/src/protocol/PacketCodec.cpp`

### 14.1 encode()

```cpp
const std::size_t bodySize = envelope.ByteSizeLong();
std::vector<std::uint8_t> packet(kHeaderSize + bodySize);
writeUint32(packet.data(), static_cast<std::uint32_t>(bodySize));
writeUint32(packet.data() + 4, messageType);
envelope.SerializeToArray(packet.data() + kHeaderSize, bodySize);
```

执行流程：

```text
计算 Protobuf 大小
  ↓
分配 8 + bodySize 字节
  ↓
写入长度
  ↓
写入消息类型
  ↓
序列化 Protobuf
```

### 14.2 最大包体限制

```cpp
kMaximumBodySize = 1024U * 1024U;
```

如果没有限制，恶意客户端可以声明一个巨大包体，诱导服务端消耗大量内存。1 MiB 限制是基础拒绝服务防护。

## 15. PacketStreamDecoder

内部保存未处理字节：

```cpp
std::vector<std::uint8_t> buffer_;
```

每次收到 TCP 数据后调用：

```cpp
decoder_.append(data, size);
```

`next()` 的判断流程：

```text
缓存少于 8 字节
  → NeedMoreData

长度为 0 或超过 1 MiB
  → InvalidLength

缓存少于 8 + body_length
  → NeedMoreData

缓存足够
  → 取出一个完整包
  → 删除已消费字节
  → PacketReady
```

这就是当前半包和粘包处理的核心。

## 16. ProtocolHandler

文件：

- `server/src/protocol/ProtocolHandler.h`
- `server/src/protocol/ProtocolHandler.cpp`

处理顺序：

```text
检查 protocol_version
  ↓
检查 request_id
  ↓
检查 message_type 是否支持
  ↓
检查包头类型与 Protobuf payload 是否一致
  ↓
处理具体请求
```

### 16.1 Ping

Ping 响应包含：

- `code=0`。
- `message=OK`。
- 原样返回请求文本。
- 服务端 Unix 毫秒时间戳。

当前 Ping 是协议测试，不是完整心跳。正式心跳还需要定时器、最后活跃时间、超时断开和失败判定。

### 16.2 DatabaseHealth

调用：

```cpp
database_->isHealthy(message);
```

实际执行 `SELECT 1` 并返回数据库名称和健康状态。

### 16.3 Login 和 Register

当前直接返回：

```text
ERROR_CODE_NOT_IMPLEMENTED
```

尚未查询数据库、校验密码或创建 Session。

## 17. 一次 Ping 的完整流程

```text
Python 测试客户端/未来 Qt 客户端
    │
    │ 创建 Envelope
    │ protocol_version = 1
    │ request_id = ping-001
    │ ping_request.text = hello
    ▼
Protobuf 序列化
    ▼
构造 8 字节包头
    ▼
TCP Socket 发送
    ▼
Ubuntu 内核 TCP 接收缓冲区
    ▼
Boost.Asio 通知 ClientConnection
    ▼
async_read_some
    ▼
PacketStreamDecoder
    │ 拼接半包
    │ 分离粘包
    ▼
PacketCodec::decodeBody
    ▼
ProtocolHandler::handle
    ▼
生成 PingResponse
    ▼
PacketCodec::encode
    ▼
writeQueue
    ▼
async_write
    ▼
TCP 返回客户端
```

## 18. 登录请求：当前流程与未来流程

### 18.1 当前实际流程

```text
客户端构造 LoginRequest
  ↓
message_type = 12
  ↓
服务端拆包和反序列化
  ↓
校验版本、request_id 和类型
  ↓
返回 1006 NOT_IMPLEMENTED
```

当前不会：

- 查询用户。
- 校验密码。
- 生成 Token。
- 创建 Session。
- 更新在线状态。

### 18.2 未来完整流程

```text
Qt 登录窗口
  ↓
客户端 AuthService 构造 LoginRequest
  ↓
NetworkClient 编码并发送
  ↓
ClientConnection 拆包
  ↓
ProtocolHandler
  ↓
服务端 AuthService（尚未实现）
  ↓
UserRepository 查询 MySQL
  ↓
读取密码哈希
  ↓
Argon2/bcrypt 校验
  ↓
生成 Session/Token
  ↓
LoginResponse
  ↓
TCP 返回
  ↓
Qt 客户端通过 request_id 找到原请求
  ↓
登录成功后进入主窗口
```

## 19. 注册请求：当前流程与未来流程

当前流程：

```text
RegisterRequest
  ↓
拆包和反序列化成功
  ↓
ProtocolHandler
  ↓
返回 NOT_IMPLEMENTED
```

未来应实现：

```text
校验用户名、密码和昵称
  ↓
检查用户名是否存在
  ↓
Argon2/bcrypt 哈希密码
  ↓
开启事务
  ↓
参数化 INSERT users
  ↓
提交事务
  ↓
返回 RegisterResponse
```

当前没有用户表、migration、参数化 SQL、密码哈希库和用户仓储。

## 20. 当前实际技术栈

### 20.1 C++

已经使用：

- C++17。
- 类、结构体和构造函数。
- `const`、引用和指针。
- `std::string`。
- `std::vector`、`std::array`、`std::deque`。
- `std::unordered_map`。
- `std::unique_ptr`、`std::shared_ptr`。
- `std::enable_shared_from_this`。
- 自定义智能指针删除器。
- `std::move`。
- Lambda。
- 异常。
- RAII。
- `std::mutex` 和 `std::lock_guard`。
- 文件流。
- `std::chrono`。
- 匿名命名空间和静态函数。

尚未使用：

- 业务多态体系。
- 工厂模式。
- 单例类。
- C++20 协程。

### 20.2 Linux

已经涉及：

- Ubuntu。
- GCC/G++。
- Shell 和环境变量。
- Linux 进程和退出码。
- TCP Socket。
- IP 与端口。
- 文件系统配置。
- Boost.Asio 底层事件通知。

尚未直接使用：

- POSIX `socket/bind/listen/accept` 函数。
- 直接调用 epoll。
- signal 信号处理。
- 多进程。
- 显式工作线程。

### 20.3 网络

已经使用：

- TCP/IP。
- 客户端/服务端模型。
- 异步 Socket。
- 自定义二进制协议。
- 网络字节序。
- TCP 半包和粘包处理。
- Protobuf 序列化。
- request_id。
- 消息类型。
- 异步写队列。

尚未使用：

- HTTP。
- UDP。
- TLS。
- 正式心跳。
- 自动重连。
- 超时管理和连接限流。

### 20.4 数据库

已经使用：

- MySQL。
- MySQL C API。
- 数据库连接和认证。
- 连接超时。
- `utf8mb4`。
- `SELECT 1`。
- MySQL 结果集。
- RAII 资源释放。

尚未使用：

- 业务 CRUD。
- 参数化查询。
- 事务。
- 索引。
- migration。
- 连接池。
- 数据库工作线程。

### 20.5 工程化

已经使用：

- CMake。
- 子目录构建。
- 静态库和可执行目标。
- `find_package` 和 pkg-config。
- Protobuf 自动生成。
- GoogleTest。
- Python 集成测试。
- Git 和 `.gitignore`。
- 模块化目录。
- 配置文件和日志封装。

## 21. 知识树

```text
C++基础
├── 【必须掌握】类、构造函数、成员变量
├── 【必须掌握】指针、引用、const
├── 【必须掌握】vector/array/deque/unordered_map
├── 【必须掌握】unique_ptr/shared_ptr
├── 【必须掌握】RAII
├── 【必须掌握】异常处理
├── 【必须掌握】Lambda
├── 【必须掌握】对象生命周期
├── 【了解即可】自定义删除器
└── 【后面再学】协程、复杂模板元编程

Linux
├── 【必须掌握】进程与退出码
├── 【必须掌握】文件和环境变量
├── 【必须掌握】IP、端口、文件描述符
├── 【必须掌握】基本 Socket 调用流程
├── 【了解即可】Asio 与 epoll 的关系
├── 【后面再学】信号和 systemd
└── 【后面再学】性能分析工具

网络编程
├── 【必须掌握】TCP 三次握手和四次挥手
├── 【必须掌握】TCP 是字节流
├── 【必须掌握】粘包和拆包
├── 【必须掌握】长度头和网络字节序
├── 【必须掌握】同步与异步 IO
├── 【必须掌握】连接对象生命周期
├── 【必须掌握】异步写队列
├── 【了解即可】epoll/Reactor
├── 【后面再学】心跳、重连和超时
└── 【后面再学】TLS、限流和背压

数据库
├── 【必须掌握】MySQL 连接过程
├── 【必须掌握】SQL 基础
├── 【必须掌握】参数化查询
├── 【必须掌握】密码哈希
├── 【必须掌握】事务
├── 【必须掌握】索引基础
├── 【了解即可】连接池原理
├── 【后面再学】主从复制
└── 【后面再学】分库分表

工程化
├── 【必须掌握】CMake target
├── 【必须掌握】头文件与源文件
├── 【必须掌握】模块化职责
├── 【必须掌握】Git 提交与同步
├── 【必须掌握】单元测试
├── 【了解即可】集成测试
├── 【了解即可】日志分级
└── 【后面再学】Docker、CI/CD、监控
```

## 22. 当前设计中做得较好的地方

1. 启动顺序明确，数据库失败时不监听端口。
2. 配置、数据库、网络和协议分开。
3. MySQL 连接与结果集使用 RAII。
4. TCP 半包和粘包有独立解码器。
5. 每个客户端拥有独立连接对象。
6. 异步写使用队列。
7. 包体最大限制为 1 MiB。
8. 协议包含版本、request_id 和错误码。
9. Windows 客户端与 Linux 服务端可独立构建。
10. 已有单元测试和集成测试基础。

## 23. 当前设计问题和潜在风险

### 23.1 单 MySQL 连接

所有请求共享一个连接，并发能力有限，连接断开后的恢复能力也不足。

### 23.2 SQL 阻塞唯一 IO 线程

`DatabaseHealth` 会在唯一 Asio IO 线程同步执行 `SELECT 1`。慢查询会阻塞所有客户端网络处理。

### 23.3 ProtocolHandler 依赖具体 DatabaseManager

协议层直接保存：

```cpp
std::shared_ptr<DatabaseManager> database_;
```

单元测试甚至使用 `ProtocolHandler handler(nullptr)`。Ping 测试可以通过，但误调用健康检查会产生空指针问题。

未来可以通过数据库健康接口或函数注入降低耦合，但当前阶段不需要为了设计模式强行复杂化。

### 23.4 没有连接超时和正式心跳

客户端连接后可以长期不发送数据，占用 Socket 资源。

### 23.5 没有优雅退出

当前没有处理 `SIGINT` 和 `SIGTERM`，Ctrl+C 时不会主动停止 accept、关闭会话和执行清理流程。

### 23.6 拆包存在复制开销

每解析一个包都会执行 `assign` 和 `erase`，可能移动缓冲区中的大量数据。学习项目可以接受，高性能版本可使用读写下标或环形缓冲区。

### 23.7 类型信息重复

包头 `message_type` 与 Protobuf `oneof` 都描述消息类型，优点是路由和校验方便，缺点是存在不一致风险，所以必须执行类型匹配检查。

### 23.8 没有 TLS

协议中存在 password 字段，但 TCP 是明文连接。在引入 TLS 前不能安全传输真实密码。

### 23.9 日志上下文不足

日志没有连接 ID 和 request_id，客户端增多后难以追踪一次完整请求。

### 23.10 异步回调异常边界不足

某些异步回调中的意外异常如果传播到 `io_context.run()`，可能导致整个服务端退出。生产代码需要更细粒度的异常保护。

## 24. 面试中的服务端架构介绍

可以这样回答：

> 我的项目是一个基于 C++17 的桌面 IM 系统，目前服务端运行在 Ubuntu，客户端使用 Qt。服务端采用模块化单体架构，网络层使用 Boost.Asio 实现异步 TCP Server。
>
> 服务启动时先通过 ConfigManager 读取监听地址和 MySQL 配置，密码通过环境变量注入。DatabaseManager 使用 MySQL C API 建立长连接，并执行 SELECT 1 验证数据库可用。只有数据库初始化成功后，服务端才开始监听端口。
>
> 网络层中 NetworkServer 负责监听和异步 accept，每个客户端对应一个 ClientConnection。连接对象通过 shared_ptr 和 enable_shared_from_this 管理异步回调期间的生命周期。
>
> TCP 是字节流，所以我设计了一个 8 字节协议头，包含 4 字节包体长度和 4 字节消息类型，包体使用 Protobuf。PacketStreamDecoder 维护接收缓冲区，可以处理半包和粘包；发送端通过写队列避免多个 async_write 重叠。
>
> ProtocolHandler 负责协议版本、request_id 和消息类型校验，目前实现了 Ping 和数据库健康检查，登录注册只完成了协议定义，还没有实现业务 SQL。
>
> 数据库连接和结果集通过 unique_ptr 加自定义删除器实现 RAII。目前采用单 IO 线程和单数据库连接，后续会增加数据库工作线程、连接池、用户仓储、密码哈希和 Session 管理。

回答时不要把尚未实现的功能说成已经完成。

## 25. 高频面试问题

### 25.1 为什么使用 TCP？

面试官为什么问：判断是否理解 IM 的可靠性需求。

回答：

> IM 的登录、好友关系和聊天消息通常要求可靠、有序传输。TCP 提供可靠传输、顺序保证、重传和流量控制，因此适合作为基础连接。TCP 只有字节流边界，所以应用层还需要处理粘包和拆包。

应体现：可靠、有序、字节流。

### 25.2 TCP 和 UDP 有什么区别？

> TCP 面向连接、可靠、有序；UDP 无连接，不保证到达和顺序，但开销较小。文字聊天适合 TCP，实时音视频可能考虑 UDP。

### 25.3 什么是粘包和拆包？

> TCP 没有消息边界。一次发送的数据可能被分多次接收，多个发送也可能合并成一次接收。当前项目通过长度字段和流式缓冲区判断完整数据包。

### 25.4 为什么需要协议头？

> 协议头告诉接收端包体长度和消息类型。长度用于划分 TCP 字节流，类型用于路由请求。

### 25.5 为什么使用网络字节序？

> 不同 CPU 可能使用不同字节序。网络协议统一使用大端字节序，保证不同机器解析结果一致。

### 25.6 Protobuf 有什么作用？

> Protobuf 根据 schema 把结构化消息序列化成紧凑二进制数据，并自动生成 C++ 类。相比手工处理字段偏移更安全，也方便客户端和服务端共享协议。

### 25.7 request_id 有什么用？

> 一个长连接上可以连续发送多个请求。响应返回相同 request_id，客户端据此找到原始请求，未来可以支持异步并发请求。

### 25.8 Socket 是什么？

> Socket 是应用程序访问操作系统网络协议栈的接口。在 Linux 中通常表现为文件描述符，可以执行 bind、listen、accept、read 和 write。

### 25.9 epoll 是什么，项目是否使用了 epoll？

> epoll 是 Linux 的 IO 多路复用机制，可以让一个线程等待多个文件描述符事件。当前代码没有直接调用 epoll，而是使用 Boost.Asio；Asio 在 Linux 上通常会使用 epoll 作为底层实现。

不要回答“项目手写了 epoll”。

### 25.10 服务端如何处理多个客户端？

> NetworkServer 持续执行 async_accept，每个客户端创建一个独立 ClientConnection，共享同一个 io_context。当前是单线程事件循环，不是一个客户端对应一个线程。

### 25.11 为什么使用 shared_from_this？

> 异步操作完成时原函数可能早已返回。回调捕获 shared_ptr 可以保证连接对象在回调执行完成前不会销毁，避免悬空指针。

### 25.12 为什么写操作需要队列？

> 同一个 Socket 上不应该重叠发起多个 async_write。写队列保证一次只发送一个缓冲区，维持响应顺序和缓冲区生命周期。

### 25.13 RAII 是什么？

> RAII 把资源生命周期绑定到对象生命周期。对象构造时获得资源，析构时释放。项目中通过 unique_ptr 自定义删除器自动调用 mysql_close 和 mysql_free_result。

### 25.14 unique_ptr 和 shared_ptr 有什么区别？

> unique_ptr 表示唯一所有权，适合管理 MySQL 连接资源；shared_ptr 表示共享所有权，适合异步连接对象和多个模块共享 DatabaseManager。

### 25.15 为什么不能每次 SQL 都重新连接？

> MySQL 连接包含 TCP 握手、协议握手和认证，开销较大。频繁连接会增加延迟和数据库压力，因此应复用连接或使用连接池。

### 25.16 当前数据库连接有什么问题？

> 当前只有一个连接，而且 SQL 在唯一 IO 线程同步执行。慢查询会阻塞全部网络事件，后续应加入数据库工作线程和连接池。

### 25.17 登录请求完整流程是什么？

> 客户端构造 LoginRequest，Protobuf 序列化后添加长度和类型头，通过 TCP 发送。服务端拆包、反序列化、校验协议和 request_id，之后应查询用户、校验密码哈希、生成 Session 并返回 LoginResponse。当前只完成协议校验，业务部分返回 NOT_IMPLEMENTED。

### 25.18 CMake 有什么作用？

> CMake 描述构建目标、源文件和依赖关系，并生成 Ninja 或 Makefile。项目中它负责客户端和服务端独立构建、查找 Boost/MySQL/Protobuf、生成 pb.cc/pb.h、链接依赖并注册测试。

### 25.19 为什么数据库失败时服务端不继续启动？

> 后续核心业务依赖数据库。如果继续监听会形成假健康状态，所以当前采用 fail-fast，数据库验证失败时直接返回非零退出码。

### 25.20 离生产环境还缺什么？

> 还缺 TLS、密码哈希、认证和 Session、业务表和 migration、数据库连接池、工作线程、心跳超时、优雅退出、限流、日志监控、离线消息以及压力测试。

## 26. 后续学习优先级

### 第一优先级：继续第四阶段前必须掌握

1. TCP 字节流、粘包和拆包。
2. Socket、bind、listen、accept、connect。
3. Boost.Asio 异步回调。
4. `shared_ptr` 和异步对象生命周期。
5. Protobuf 的 message、enum 和 oneof。
6. MySQL 基础 SQL。
7. 参数化查询。
8. Argon2/bcrypt 密码哈希。
9. MySQL 事务基础。
10. 用户表设计。

达到以下程度就可以继续做注册：

- 能画出注册请求完整链路。
- 能写参数化 INSERT。
- 能解释密码为什么不能明文保存。
- 能正确处理用户名重复。
- 能使用事务保证数据一致性。
- 能返回明确错误码。

### 第二优先级：完成登录和多客户端业务

1. 数据库工作线程。
2. 基础连接池。
3. Session 和 Token。
4. 线程安全。
5. 在线连接映射。
6. 超时和心跳。
7. Qt `QTcpSocket`。
8. Qt 客户端协议编解码。

### 第三优先级：面试提升和生产化

1. epoll 原理。
2. Reactor 模型。
3. TLS。
4. 性能测试。
5. 日志轮转和监控。
6. Redis 在线状态或缓存。
7. 离线消息。
8. Docker 和 CI/CD。
9. MySQL 索引优化。
10. 服务端优雅关闭。

## 27. 第三阶段真正学到的内容

第三阶段最有价值的不是代码行数，而是把多个知识点连接成一条真实链路：

```text
CMake 查找依赖并生成代码
    ↓
Linux 启动 C++ 进程
    ↓
读取配置和环境变量
    ↓
通过 RAII 管理 MySQL 连接
    ↓
执行 SQL 健康检查
    ↓
创建 TCP 监听器
    ↓
异步接受多个客户端
    ↓
处理 TCP 字节流
    ↓
解决半包和粘包
    ↓
反序列化 Protobuf
    ↓
校验协议版本、消息类型和 request_id
    ↓
生成响应
    ↓
通过异步写队列返回客户端
```

这一阶段真正接触到了：

- C++ 对象生命周期。
- 智能指针和 RAII。
- 异步回调。
- TCP 字节流与消息边界。
- 自定义应用层协议。
- Protobuf 序列化。
- MySQL 连接过程。
- 配置和敏感信息分离。
- 模块化与依赖关系。
- CMake 目标化构建。
- 单元测试和集成测试。

下一阶段最适合实现的第一个完整业务闭环是：

```text
注册请求
  ↓
参数校验
  ↓
密码哈希
  ↓
MySQL 参数化 INSERT
  ↓
错误处理
  ↓
RegisterResponse
  ↓
Qt 客户端展示结果
```

完成这个闭环后，项目才会从基础设施框架进入真正的 IM 业务开发。

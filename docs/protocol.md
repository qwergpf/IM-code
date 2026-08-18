# IM 协议框架 v1

## 1. 传输帧

服务端基于 TCP。TCP 是字节流，不保留发送端的消息边界，因此每个 Protobuf 消息使用固定长度头：

```text
4 字节无符号大端整数：Protobuf 消息体长度
N 字节：序列化后的 im.protocol.v1.Envelope
```

- 长度头使用网络字节序。
- 消息体不能为空。
- 单个消息体最大为 1 MiB。
- 长度为 0 或超过上限时，服务端关闭连接。
- 服务端使用精确长度异步读取，因此能够处理 TCP 半包和粘包。

## 2. Envelope

协议定义位于 `server/protocol/im_protocol.proto`，顶层消息统一包含：

- `protocol_version`：当前固定为 `1`。
- `request_id`：请求唯一标识，响应必须原样回传。
- `payload`：通过 Protobuf `oneof` 保存具体请求或响应。

当前消息类型：

| 消息 | 方向 | 作用 |
| --- | --- | --- |
| `PingRequest` | 客户端 → 服务端 | 验证 TCP 和 Protobuf 链路 |
| `PingResponse` | 服务端 → 客户端 | 回显文本和服务端时间 |
| `DatabaseHealthRequest` | 客户端 → 服务端 | 检查 PostgreSQL 连接 |
| `DatabaseHealthResponse` | 服务端 → 客户端 | 返回非敏感数据库健康状态 |
| `ErrorResponse` | 服务端 → 客户端 | 返回稳定错误码和可读错误信息 |

## 3. 错误码

| 错误码 | 名称 | 含义 |
| --- | --- | --- |
| `1001` | `MALFORMED_MESSAGE` | Protobuf 消息无法解析；响应后关闭连接 |
| `1002` | `UNSUPPORTED_PROTOCOL_VERSION` | 不支持请求中的协议版本 |
| `1003` | `MISSING_REQUEST_ID` | 请求没有携带 `request_id` |
| `1004` | `UNSUPPORTED_MESSAGE_TYPE` | 客户端发送了不支持的消息类型 |
| `2001` | `DATABASE_UNAVAILABLE` | 为后续数据库业务操作预留 |

数据库健康检查本身返回 `DatabaseHealthResponse{healthy=false}`，说明文本固定为非敏感信息，不返回连接字符串、账号、密码或底层 SQL 错误。

## 4. 扩展规则

- 新请求和响应必须在 `Envelope.oneof payload` 中使用新的字段编号。
- 已发布字段编号不得复用或更改含义。
- 所有请求必须验证协议版本和 `request_id`。
- 新增或修改消息时必须同步更新 C++ 单元测试、Python 冒烟测试和本文档。
- 文件传输不能把大文件直接塞进单个 Envelope；后续应使用文件元数据消息与独立分片传输流程。

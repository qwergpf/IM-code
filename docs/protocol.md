# IM 服务端协议

## TCP 封包

TCP 是字节流，不能假设一次 `recv` 就对应一个完整消息。每个数据包使用网络字节序的大端格式：

```text
4 字节 body_length
4 字节 message_type
body_length 字节 Protobuf Envelope
```

单个 body 最大为 1 MiB。零长度或超限数据包会被拒绝。服务端的 `PacketStreamDecoder` 支持半包和粘包。

## 协议消息

当前协议定义在 `server/protocol/im_protocol.proto`，版本为 `1`，可处理：

- Ping 请求和响应
- MySQL 健康检查请求和响应
- 注册、登录协议占位消息
- 统一错误响应

每个请求必须有非空 `request_id`，响应会返回相同值。包头 `message_type` 必须与 `Envelope` 中的 payload 类型一致。

## 错误码

| 错误码 | 含义 |
| ---: | --- |
| 0 | 成功 |
| 1001 | 数据包或 Protobuf 无法解析 |
| 1002 | 不支持的协议版本 |
| 1003 | 缺少 request_id |
| 1004 | 不支持的消息类型 |
| 1005 | 包头类型与 Protobuf payload 不一致 |
| 1006 | 当前阶段未实现 |
| 2001 | MySQL 不可用 |

注册和登录目前只定义消息，不处理真实业务；在加入 TLS 前不要通过该协议传输真实密码。

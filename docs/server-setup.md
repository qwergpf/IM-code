# Ubuntu 服务端启动指南

## 安装依赖

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build pkg-config \
  libboost-system-dev protobuf-compiler libprotobuf-dev \
  default-libmysqlclient-dev mysql-server libgtest-dev \
  python3 python3-protobuf
```

## 初始化 MySQL

在本地开发环境执行：

```sql
CREATE DATABASE im_chat CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER 'im_app'@'127.0.0.1' IDENTIFIED BY '替换为本地开发密码';
GRANT ALL PRIVILEGES ON im_chat.* TO 'im_app'@'127.0.0.1';
FLUSH PRIVILEGES;
```

密码只通过环境变量提供：

```bash
export IM_DB_PASSWORD='替换为本地开发密码'
```

## 构建和启动

```bash
cd /home/gpf/IM_sever_code
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
  -DIM_BUILD_CLIENT=OFF -DIM_BUILD_SERVER=ON -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/server --config ./build/server.conf
```

默认监听 `0.0.0.0:8888`。可使用 `./build/server --config ./build/server.conf` 指定配置文件。

## 冒烟测试

服务端运行后，在另一个终端执行：

```bash
python3 server/tests/integration/smoke_test.py \
  --host 127.0.0.1 --port 8888 --proto-build-dir build/server-build
```

测试脚本需要在构建目录下生成的 `im_protocol_pb2.py` 所在目录运行；它会验证 Ping 半包、多个连续数据包、数据库健康响应和 request_id 保留。

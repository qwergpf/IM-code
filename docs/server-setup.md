# Ubuntu 服务端与 PostgreSQL 启动指南

本文用于在 Ubuntu 22.04.4 虚拟机中构建并运行 IM 服务端最小闭环。

## 1. 安装依赖

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build \
  libboost-system-dev libpqxx-dev \
  protobuf-compiler libprotobuf-dev \
  libgtest-dev \
  postgresql postgresql-contrib \
  python3 python3-protobuf
```

## 2. 初始化开发数据库

打开 PostgreSQL 控制台：

```bash
sudo -u postgres psql
```

执行以下 SQL，并把示例密码替换为仅用于本地开发的密码：

```sql
CREATE ROLE im_app LOGIN PASSWORD '<本地开发密码>';
CREATE DATABASE im_chat OWNER im_app;
\q
```

确认数据库服务运行：

```bash
sudo systemctl enable --now postgresql
sudo systemctl status postgresql
```

## 3. 获取代码并构建

```bash
git clone https://github.com/qwergpf/IM-code.git
cd IM-code

cmake -S . -B build-server -G Ninja \
  -DIM_BUILD_CLIENT=OFF \
  -DIM_BUILD_SERVER=ON \
  -DBUILD_TESTING=ON

cmake --build build-server
ctest --test-dir build-server --output-on-failure
```

如果已经克隆仓库：

```bash
git pull
```

## 4. 配置并启动服务端

真实密码只放在当前终端环境变量中，不要写进 Git：

```bash
export IM_DB_PASSWORD='<本地开发密码>'
export IM_SERVER_HOST='0.0.0.0'
export IM_SERVER_PORT='9000'
export IM_DB_HOST='127.0.0.1'
export IM_DB_PORT='5432'
export IM_DB_NAME='im_chat'
export IM_DB_USER='im_app'

./build-server/server/im_server
```

启动成功时应看到：

```text
IM server listening on 0.0.0.0:9000 using database im_chat
```

数据库密码错误或 PostgreSQL 不可用时，程序会返回非零退出码，并且不会开始监听端口。

## 5. 运行集成测试

另开一个 Ubuntu 终端：

```bash
cd IM-code
python3 server/tests/integration/smoke_test.py \
  --host 127.0.0.1 \
  --port 9000 \
  --proto-build-dir build-server
```

成功时输出：

```text
IM server smoke test passed
```

脚本会验证：

- 拆分发送一个帧，确认半包处理。
- 一次发送两个帧，确认粘包处理。
- Ping 请求和响应。
- PostgreSQL `SELECT 1` 健康检查。

验证运行时数据库故障时，保持服务端运行，在另一个终端停止 PostgreSQL：

```bash
sudo systemctl stop postgresql
python3 server/tests/integration/smoke_test.py \
  --host 127.0.0.1 \
  --port 9000 \
  --expect-db-unhealthy
sudo systemctl start postgresql
```

该模式要求健康响应为 `healthy=false`，并检查响应中不包含密码或连接字符串字段。

## 6. 虚拟机网络

查看 Ubuntu IP：

```bash
ip address
```

如果启用了 UFW，开放开发端口：

```bash
sudo ufw allow 9000/tcp
```

Windows 客户端后续应连接虚拟机的实际 IP 和端口 9000。生产部署时应收紧防火墙来源范围并配置 TLS。

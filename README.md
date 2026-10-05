# 基于 C++11 的高并发数据库连接池

一个从零实现的 MySQL 连接池示例项目，使用 `C++11 + MySQL C API + CMake` 构建。

当前项目的推荐运行方式是：

- 本地写代码
- 本地编译 `test_pool`
- 本地运行 `./bin/test_pool`
- Docker 只负责启动 MySQL

## 项目特点

- 使用 `std::shared_ptr<MySQL>` 和自定义删除器实现连接自动归还
- 使用生产者 / 消费者双条件变量降低高并发下的无效唤醒
- 支持按配置预热连接、按需补充连接
- 支持配置文件解析与合法性校验

## 目录结构

```text
.
├── CMakeLists.txt
├── docker-compose.yml
├── autobuild.sh
├── config/
│   └── connection_pool.conf.example
├── docker/mysql/init/
│   └── 01-init-connection-pool.sql
├── example/
│   └── main.cpp
├── include/
└── src/
```

## 开发前提

本机建议准备：

- C++11 编译器
- CMake 3.10+
- MySQL 客户端开发库，例如 Ubuntu 上的 `libmysqlclient-dev`

## 配置说明

程序实际读取的配置文件是 `config/connection_pool.conf`，仓库中保留的是模板文件 `config/connection_pool.conf.example`。

第一次本地运行前先复制模板：

```bash
cp config/connection_pool.conf.example config/connection_pool.conf
```

模板默认用于宿主机本地运行：

```conf
ip=127.0.0.1
port=3306
username=connection_pool_app
password=wang112233
dbname=connection_pool_dev
```

地址约定：

- 宿主机本地运行 `./bin/test_pool` 时使用 `127.0.0.1:3306`
- `mysql_db` 是 Docker Compose 网络里的 MySQL 服务名，只用于容器网络内部寻址说明

## 启动 MySQL

当前 `docker-compose.yml` 只负责：

- `mysql_db`
- `mysql_data`

启动：

```bash
docker compose up -d mysql_db
```

查看状态：

```bash
docker compose ps
```

查看日志：

```bash
docker compose logs -f mysql_db
```

停止：

```bash
docker compose down
```

如果你修改了初始化 SQL、数据库用户名或密码，希望让 MySQL 从零重新初始化：

```bash
docker compose down -v
docker compose up -d mysql_db
```

如果之前用旧版编排起过 `cpp_app`，看到 orphan 提示时可以清理一次：

```bash
docker compose down --remove-orphans
```

## 本地构建

执行：

```bash
./autobuild.sh
```

或者手动执行：

```bash
cmake -S . -B build
cmake --build build
```

产物：

- `bin/test_pool`

## 本地运行

确认 `mysql_db` 已启动并且 `config/connection_pool.conf` 已存在后，直接运行：

```bash
./bin/test_pool
```

`example/main.cpp` 当前主要用于并发借还连接测试，重点验证“连接能否成功建立与复用”；它不依赖额外业务表结构，因此空数据库也可以跑通。

## 初始化内容

MySQL 首次启动时会执行：

- [docker/mysql/init/01-init-connection-pool.sql](docker/mysql/init/01-init-connection-pool.sql)

它会创建：

- 数据库：`connection_pool_dev`
- 用户：`connection_pool_app`
- 密码：`wang112233`

## 成功运行的标志

当你看到以下现象时，说明项目已经跑通：

- `mysql_db` 处于运行中，健康检查最终为 `healthy`
- 本地 `./bin/test_pool` 输出多条“成功拿到连接”
- 程序正常结束

## 推荐顺序

第一次：

```bash
cd ~/projects/cpp_project/06_connection_pool
cp config/connection_pool.conf.example config/connection_pool.conf
docker compose up -d mysql_db
./autobuild.sh
./bin/test_pool
```

日常开发：

```bash
cd ~/projects/cpp_project/06_connection_pool
docker compose up -d mysql_db
./autobuild.sh
./bin/test_pool
```

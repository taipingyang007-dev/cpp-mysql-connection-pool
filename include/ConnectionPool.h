#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include <memory>
#include <thread>
#include <atomic>

#include "MySQL.h"
#include "ConnectionPoolConfig.h"

class ConnectionPool
{
public:
    // 使用单列模式，获取车库的唯一大管家
    static ConnectionPool *getConnectionPool();

    // 消费者接口：业务线程来借车
    std::shared_ptr<MySQL> getConnection();

private:
    // 将构造函数私有化，禁止外部创建对象
    ConnectionPool();
    // 将析构函数私有化，禁止外部销毁对象
    ~ConnectionPool();

    // 禁用拷贝构造函数
    ConnectionPool(const ConnectionPool &) = delete;
    // 禁用赋值运算符
    ConnectionPool &operator=(const ConnectionPool &) = delete;

    // 生产者接口：后台一条专门负责造新车的流水线
    void produceConnectionTask();

    // 按配置创建一个可用连接
    MySQL *createConnection();

    // ================= 核心部件 =================

    // 车库：存放所有空闲、可用的MySQL连接
    std::queue<MySQL *> _connectionQueue;

    // 保安：维护车库队列线程安全的互斥锁
    std::mutex _queueMutex;

    // VIP 休息室：用于生产者线程间通信的条件变量
    std::condition_variable _cv_producer;
    // VIP 休息室：用于消费者线程间通信的条件变量
    std::condition_variable _cv_consumer;
    // 等待所有借出连接归还的条件变量
    std::condition_variable _cv_shutdown;

    // 当前借出去、还没归还的连接数
    int _borrowedCnt = 0;

    // 电源总开关：原子操作，保证多线程读写的绝对安全
    std::atomic<bool> _isRunning;

    // 把线程对象存起来
    std::thread _produceThread;

    // ================= 配置参数 =================

    // 独立配置对象
    ConnectionPoolConfig _config;

    // 当前总连接数
    int _connectionCnt = 0;
};
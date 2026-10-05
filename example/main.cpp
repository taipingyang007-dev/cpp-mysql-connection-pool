#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

#include "../include/ConnectionPool.h"

// 模拟单个用户的业务操作
void userBusinessLogic(int userId) {
    // 1. 获取唯一的单例车库
    ConnectionPool* pool = ConnectionPool::getConnectionPool();

    // 2. 从车库借一辆车（拿到智能合同）
    std::shared_ptr<MySQL> conn = pool->getConnection();

    if (conn != nullptr) {
        // 模拟业务处理耗时：假设每次查询数据库需要花费 10 毫秒
        // printf 在多线程下比 cout 打印更整齐一点
        printf("用户 [%d] 成功拿到连接，正在执行数据库查询...\n", userId);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } else {
        printf("用户 [%d] 获取连接失败！\n", userId);
    }

    // 3. 函数结束，智能指针 conn 死亡，触发自动还车机制！
}

int main() {
    std::cout << "========== 聊天服务器并发连接测试开始 ==========" << std::endl;

    // 记录测试开始时间
    auto startTime = std::chrono::high_resolution_clock::now();

    // 创建一个存放 50 个线程的容器
    std::vector<std::thread> threads;

    // 瞬间爆发出 50 个并发线程，模拟高并发洪水
    for (int i = 1; i <= 50; ++i) {
        threads.push_back(std::thread(userBusinessLogic, i));
    }

    // 主线程等待所有业务线程全部执行完毕
    for (auto& t : threads) {
        t.join();
    }

    // 记录测试结束时间
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "========== 测试结束 ==========" << std::endl;
    std::cout << "50 个并发请求全部处理完毕，总耗时: " << duration.count() << " 毫秒" << std::endl;

    return 0;
}

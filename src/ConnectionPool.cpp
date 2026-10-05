#include "ConnectionPool.h"

// 使用单列模式，获取车库的唯一大管家
ConnectionPool *ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool{};
    return &pool;
}

// 构造函数
ConnectionPool::ConnectionPool()
{
    if (!_config.loadConfig("./config/connection_pool.conf"))
    {
        std::cerr << "连接池配置加载失败，程序退出！" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const PoolConfig &poolConfig = _config.getPoolConfig();

    for (int i = 0; i < poolConfig.initSize; ++i)
    {
        MySQL *conn = createConnection();
        if (conn == nullptr)
        {
            std::cerr << "MySQL 初始化连接失败，程序退出！" << std::endl;
            std::exit(EXIT_FAILURE);
        }

        _connectionQueue.push(conn);
        ++_connectionCnt;
    }

    _isRunning = true; // 打开线程的电源总开关

    // 启动线程，并保存到_produceThread成员变量中
    _produceThread = std::thread([this]()
                                 { produceConnectionTask(); });
}

// 析构函数
ConnectionPool::~ConnectionPool()
{
    _isRunning = false;        // 关闭线程的电源总开关
    _cv_producer.notify_all(); // 唤醒生产者线程，让它有机会看到_isRunning的变化，及时退出
    _cv_consumer.notify_all(); // 唤醒消费者线程，让它有机会看到_isRunning的变化，及时退出

    if (_produceThread.joinable())
    {
        _produceThread.join(); // 等待生产者线程结束
    }

    std::unique_lock<std::mutex> lock(_queueMutex);
    // 等待所有借出去的连接都处理完毕，确保没有线程在使用连接了
    _cv_shutdown.wait(lock, [this]()
                       { return _borrowedCnt == 0; });

    // 清理车库里的车
    while (!_connectionQueue.empty())
    {
        MySQL *conn = _connectionQueue.front();
        _connectionQueue.pop();
        delete conn; // 销毁车辆
    }
}

// 消费者接口：业务线程来借车
std::shared_ptr<MySQL> ConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(_queueMutex);
    const PoolConfig &poolConfig = _config.getPoolConfig();

    bool ok = _cv_consumer.wait_for(
        lock,
        std::chrono::milliseconds(poolConfig.connectionTimeout),
        [this]()
        {
            return !_connectionQueue.empty() || !_isRunning;
        });

    // 超时了
    if (!ok)
    {
        return nullptr;
    }

    // 连接池关闭了
    if (!_isRunning)
    {
        return nullptr;
    }

    MySQL *conn = _connectionQueue.front();
    _connectionQueue.pop();

    // 这里表示“真的借出去了一个连接”
    ++_borrowedCnt;

    if (static_cast<int>(_connectionQueue.size()) < poolConfig.initSize)
    {
        _cv_producer.notify_one(); // 唤醒生产者线程，让它有机会看到车库空了，赶紧造车
    }

    // 【终极魔法：自定义 shared_ptr 删除器】
    // 正常情况下，shared_ptr 离开作用域会执行 delete conn，把车砸了。
    // 我们在这里截胡！给它传一个 Lambda 表达式。
    // 告诉它：你死的时候，别砸车，把车 push 回队列里！
    std::shared_ptr<MySQL> sp(conn, [this](MySQL *ptr)
                              {
                                  std::unique_lock<std::mutex> lock(_queueMutex);

                                  // 如果连接池还活着，就归还到队列
                                  if (_isRunning)
                                  {
                                      _connectionQueue.push(ptr);
                                      _cv_consumer.notify_one();
                                  }
                                  else
                                  {
                                      // 如果连接池已经开始析构，就别再放回队列了，直接销毁
                                      delete ptr;
                                  }

                                // 不管是归还还是销毁，这个“借出的连接”都算处理完了
                                --_borrowedCnt; 
                                // 如果已经没有借出的连接了，通知析构函数可以继续往下走
                                if (_borrowedCnt == 0)
                                {
                                    _cv_shutdown.notify_one();
                                } });
    return sp;
}

// 生产者接口：后台一条专门负责造新车的流水线
void ConnectionPool::produceConnectionTask()
{
    const PoolConfig &poolConfig = _config.getPoolConfig();
    while (_isRunning)
    {
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            while ((_connectionQueue.size() >= static_cast<size_t>(poolConfig.initSize) || _connectionCnt >= poolConfig.maxSize) && _isRunning)
            {
                _cv_producer.wait(lock);
            }

            if (!_isRunning)
            {
                break;
            }
        }

        // 锁外做慢操作
        MySQL *conn = createConnection();
        if (conn == nullptr)
        {
            continue;
        }

        {
            std::unique_lock<std::mutex> lock(_queueMutex);

            if (!_isRunning)
            {
                delete conn;
                break;
            }

            // 重新检查一次，避免回来时库存已经够了
            if (_connectionQueue.size() >= static_cast<size_t>(poolConfig.initSize) || _connectionCnt >= poolConfig.maxSize)
            {
                delete conn;
                continue;
            }

            _connectionQueue.push(conn);
            ++_connectionCnt;
        }

        _cv_consumer.notify_one();
    }
}

// 抽出来复用：按配置创建一个可用连接
MySQL *ConnectionPool::createConnection()
{
    const DbConfig &dbConfig = _config.getDbConfig();

    MySQL *conn = new MySQL();

    if (conn->connect(dbConfig.ip,
                      dbConfig.port,
                      dbConfig.username,
                      dbConfig.password,
                      dbConfig.dbname))
    {
        return conn;
    }

    delete conn;
    return nullptr;
}
#pragma once

#include <string>

// 数据库的连接配置
struct DbConfig
{
    std::string ip;
    unsigned short port;
    std::string username;
    std::string password;
    std::string dbname;
};

// 连接池运行配置
struct PoolConfig
{
    int initSize;              // 连接池初始连接数
    int maxSize;               // 连接池最大连接数
    int connectionTimeout = 0; // 获取连接的最大等待时间，单位：毫秒
};

class ConnectionPoolConfig
{
public:
    // 从配置文件中加载配置
    bool loadConfig(const std::string &path);

    // 获取数据库配置
    const DbConfig &getDbConfig() const;

    // 获取连接池配置
    const PoolConfig &getPoolConfig() const;

private:
    // 给不同的key赋值，路由分发
    bool assignConfigItem(const std::string &key, const std::string &value);

    // 检查配置是否合法
    /*
        这是企业级代码的标志。读完文件不算完，必须做合法性校验！
        比如检查 port 是不是在 1~65535 之间？initSize 是不是大于 0？
        如果不做这个校验，带着错乱的配置去连数据库，排查 bug 会让你生不如死。
    */
    bool validate() const;

    // 去掉字符串首尾空白
    static std::string trim(const std::string &text);

    // 把字符串解析成 int
    static bool parseInt(const std::string &text, int &value);

private:
    DbConfig _dbConfig;
    PoolConfig _poolConfig;

    // 这些标记位是为了区分“字段缺失”和“字段值为空/为0”
    bool _hasIp = false;
    bool _hasPort = false;
    bool _hasUsername = false;
    bool _hasPassword = false;
    bool _hasDbName = false;
    bool _hasInitSize = false;
    bool _hasMaxSize = false;
    bool _hasConnectionTimeout = false;

    /*
        为什么要这些 _hasXxx 标记？
        因为你不能只靠默认值判断字段是否缺失。
        比如 password 可能允许为空字符串，但“没有这个字段”和“字段存在但为空”不是一回事。
    */
};
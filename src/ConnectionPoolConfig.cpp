#include "ConnectionPoolConfig.h"
#include <fstream>
#include <iostream>

// 从配置文件中加载配置
bool ConnectionPoolConfig::loadConfig(const std::string &path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open())
    {
        std::cerr << "打开配置文件失败: " << path << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(ifs, line))
    {
        ++lineNumber;
        // 解析每一行配置

        std::string trimmedLine = trim(line);

        // 跳过空行和注释行
        if (trimmedLine.empty() || trimmedLine[0] == '#')
        {
            continue;
        }

        std::size_t pos = trimmedLine.find('=');
        if (pos == std::string::npos)
        {
            std::cerr << "配置文件格式错误，第 " << lineNumber << " 行缺少 '=' : "
                      << trimmedLine << std::endl;
            return false;
        }

        std::string key = trim(trimmedLine.substr(0, pos));    // 代表从0开始，长度为 pos 的字符串
        std::string value = trim(trimmedLine.substr(pos + 1)); // 代表从 pos+1 开始，长度为剩余字符串的字符串

        if (key.empty())
        {
            std::cerr << "配置文件格式错误，第 " << lineNumber << " 行 key 为空" << std::endl;
            return false;
        }

        if (!assignConfigItem(key, value))
        {
            std::cerr << "配置项非法，第 " << lineNumber << " 行: " << trimmedLine << std::endl;
            return false;
        }
    }
    return validate();
}

// 获取数据库配置
const DbConfig &ConnectionPoolConfig::getDbConfig() const { return _dbConfig; }

// 获取连接池配置
const PoolConfig &ConnectionPoolConfig::getPoolConfig() const { return _poolConfig; }

// 给不同的key赋值，路由分发
bool ConnectionPoolConfig::assignConfigItem(const std::string &key, const std::string &value)
{
    if (key == "ip")
    {
        _dbConfig.ip = value;
        _hasIp = true;
        return true;
    }

    if (key == "port")
    {
        int port = 0;
        if (!parseInt(value, port) || port <= 0 || port > 65535)
        {
            std::cerr << "无效的 port 值: " << value << std::endl;
            return false;
        }

        // 端口号是无符号短整数，但 parseInt 解析成了 int，所以这里要做类型转换
        _dbConfig.port = static_cast<unsigned short>(port);
        _hasPort = true;
        return true;
    }

    if (key == "username")
    {
        _dbConfig.username = value;
        _hasUsername = true;
        return true;
    }

    if (key == "password")
    {
        _dbConfig.password = value;
        _hasPassword = true;
        return true;
    }

    if (key == "dbname")
    {
        _dbConfig.dbname = value;
        _hasDbName = true;
        return true;
    }

    if (key == "initSize")
    {
        int initSize = 0;
        if (!parseInt(value, initSize) || initSize <= 0)
        {
            std::cerr << "无效的 initSize 值: " << value << std::endl;
            return false;
        }
        _poolConfig.initSize = initSize;
        _hasInitSize = true;
        return true;
    }

    if (key == "maxSize")
    {
        int maxSize = 0;
        if (!parseInt(value, maxSize) || maxSize <= 0)
        {
            std::cerr << "无效的 maxSize 值: " << value << std::endl;
            return false;
        }
        _poolConfig.maxSize = maxSize;
        _hasMaxSize = true;
        return true;
    }

    if (key == "connectionTimeout")
    {
        int timeout = 0;
        if (!parseInt(value, timeout) || timeout <= 0)
        {
            return false;
        }

        _poolConfig.connectionTimeout = timeout;
        _hasConnectionTimeout = true;
        return true;
    }

    // 出现未知 key，直接判定非法，避免配置写错却悄悄失效
    return false;
}

// 检查配置是否合法
/*
    这是企业级代码的标志。读完文件不算完，必须做合法性校验！
    比如检查 port 是不是在 1~65535 之间？initSize 是不是大于 0？
    如果不做这个校验，带着错乱的配置去连数据库，排查 bug 会让你生不如死。
*/
bool ConnectionPoolConfig::validate() const
{
    if (!_hasIp || _dbConfig.ip.empty())
    {
        std::cerr << "缺少合法配置项: ip" << std::endl;
        return false;
    }

    if (!_hasPort)
    {
        std::cerr << "缺少合法配置项: port" << std::endl;
        return false;
    }

    if (!_hasUsername || _dbConfig.username.empty())
    {
        std::cerr << "缺少合法配置项: username" << std::endl;
        return false;
    }

    if (!_hasPassword)
    {
        std::cerr << "缺少配置项: password" << std::endl;
        return false;
    }

    if (!_hasDbName || _dbConfig.dbname.empty())
    {
        std::cerr << "缺少合法配置项: dbname" << std::endl;
        return false;
    }

    if (!_hasInitSize || _poolConfig.initSize <= 0)
    {
        std::cerr << "缺少合法配置项: initSize" << std::endl;
        return false;
    }

    if (!_hasMaxSize || _poolConfig.maxSize < _poolConfig.initSize)
    {
        std::cerr << "缺少合法配置项或 maxSize < initSize" << std::endl;
        return false;
    }

    if (!_hasConnectionTimeout || _poolConfig.connectionTimeout <= 0)
    {
        std::cerr << "缺少合法配置项: connectionTimeout" << std::endl;
        return false;
    }

    return true;
}

// 去掉字符串首尾空白
std::string ConnectionPoolConfig::trim(const std::string &text)
{
    std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
    {
        return "";
    }

    std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

// 把字符串解析成 int
bool ConnectionPoolConfig::parseInt(const std::string &text, int &value)
// text：你要解析的字符串，比如 "3306"
// value：如果解析成功，就把结果写进这个变量里
{
    try
    {
        std::size_t pos = 0;
        int number = std::stoi(text, &pos);
        /*  stoi 的意思就是：string to int。
                它会尝试把字符串前面能解析的部分变成整数。
            例如：
                "3306" -> 3306
                "123abc" -> 它会先吃出 123
                "abc" -> 直接失败，抛异常
        */

        // 如果没把整个字符串吃完，说明不是纯数字
        if (pos != text.size())
        {
            return false;
        }

        value = number;
        return true;
    }
    catch (...)
    {
        return false;
    }
}
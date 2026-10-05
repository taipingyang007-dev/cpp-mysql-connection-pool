#include "../include/MySQL.h"

// 初始化数据库连接
MySQL::MySQL()
{
    // mysql_init()：初始化一个 MYSQL 对象，准备连接数据库
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
    }
}

// 数据库连接
bool MySQL::connect(std::string ip, unsigned short port,
                    std::string user, std::string password, std::string dbname)
{
    /*
        MYSQL *mysql, const char *host,
              const char *user, const char *passwd,
              const char *db, unsigned int port,
              const char *unix_socket,
              unsigned long clientflag
    */
                        if ( mysql_real_connect(_conn, ip.c_str(), user.c_str(), password.c_str(),dbname.c_str(),port,nullptr,0) != nullptr )
                        {
                            // 设置字符集，解决中文乱码问题
                            mysql_set_character_set(_conn, "utf8");
                            return true;
                        } else
                        {
                            return false;
                        }
}

// 更新数据库
bool MySQL::update(std::string sql) {
    if (mysql_query(_conn, sql.c_str()))
    {
        return false;
    }
    return true;
}

// 查询数据库
MYSQL_RES *MySQL::query(std::string sql) {
    if (mysql_query(_conn, sql.c_str()))
    {
        return nullptr;
    }
    return mysql_store_result(_conn);
}
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和使用语言
project(connection_pool LANGUAGES CXX)

# 明确指定使用 C++11 标准
set(CMAKE_CXX_STANDARD 11)

# 要求编译器严格按 C++11 编译
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 关闭编译器的 GNU 扩展，保持标准一致性
set(CMAKE_CXX_EXTENSIONS OFF)

# 把最终生成的可执行文件统一输出到 bin 目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 把连接池核心源码编译成可复用的库
add_library(connection_pool
    src/ConnectionPool.cpp
    src/MySQL.cpp
    src/ConnectionPoolConfig.cpp
)

# 告诉使用者头文件在 include 目录下
target_include_directories(connection_pool PUBLIC include)

# 给核心库链接线程库和 MySQL 客户端库
target_link_libraries(connection_pool PUBLIC pthread mysqlclient)

# 编译示例程序，生成 test_pool 可执行文件
add_executable(test_pool
    example/main.cpp
)

# 让示例程序链接连接池核心库
target_link_libraries(test_pool PRIVATE connection_pool)

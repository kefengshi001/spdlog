cmake_minimum_required(VERSION 3.10)
project(spdlog_stage4_porting CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog REQUIRED)
find_package(Threads REQUIRED)

# Create logs output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)

enable_testing()

# 各子模块（每个模块有自己的 CMakeLists.txt）
add_subdirectory(log)           # 日志库
add_subdirectory(algorithm)     # 算法代码
add_subdirectory(examples)      # 演示程序
add_subdirectory(tests)         # 测试

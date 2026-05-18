# 指定 CMake 最低版本要求，低于 3.10 会报错
cmake_minimum_required(VERSION 3.10)

# 定义项目名称为 spdlog_stage1_setup，使用的语言是 C++
project(spdlog_stage1_setup CXX)

# 设置 C++ 标准为 C++17
set(CMAKE_CXX_STANDARD 17)

# 强制要求编译器必须支持 C++17，否则直接报错（而不是降级）
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- 获取 spdlog 依赖 ----
find_package(spdlog)

# # 加载 CMake 内置的 FetchContent 模块，提供从网络下载第三方库的能力
# include(FetchContent)

# # 声明一个名为 spdlog 的外部依赖
# FetchContent_Declare(
#     spdlog
#     GIT_REPOSITORY https://github.com/gabime/spdlog.git  # 从 GitHub 克隆源码
#     GIT_TAG v1.17.0                                        # 锁定到 v1.17.0 版本
# )

# # 执行下载，并将 spdlog 作为子目录加入当前构建（等价于 add_subdirectory）
# # 下载后的源码缓存在 build 目录的 _deps/spdlog-src/ 下
# FetchContent_MakeAvailable(spdlog)

# ---- 创建日志输出目录 ----

# 在构建目录下创建 logs/ 文件夹，供程序运行时写入日志文件
# 等价于 shell 命令：mkdir -p build/logs
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs)

# ---- 构建示例程序 ----

# 定义一个可执行目标 hello_world，源文件是 src/hello_world.cpp
add_executable(hello_world src/hello_world.cpp)

# 将 spdlog 链接到 hello_world
# PRIVATE 表示 spdlog 只在 hello_world 内部使用，不会传递给依赖 hello_world 的其他目标
target_link_libraries(hello_world PRIVATE spdlog::spdlog)

# 定义第二个示例程序 multi_sink_demo
add_executable(multi_sink_demo src/multi_sink_demo.cpp)
target_link_libraries(multi_sink_demo PRIVATE spdlog::spdlog)

# ---- 测试部分 ----

# 启用 CTest 测试框架，之后可以用 add_test() 注册测试用例
enable_testing()

# 定义测试可执行程序 test_install
add_executable(test_install tests/test_install.cpp)
target_link_libraries(test_install PRIVATE spdlog::spdlog)

# 为 test_install 添加编译宏定义 LOG_DIR，值为 build/logs 的绝对路径
# 在 C++ 代码中可以通过 LOG_DIR 宏直接引用这个路径
target_compile_definitions(test_install PRIVATE
    LOG_DIR="${CMAKE_BINARY_DIR}/logs"
)

# 将 test_install 注册为一个 CTest 测试用例，名称为 test_install
# 之后可以通过 ctest 或 cmake --build . --test 运行它
add_test(NAME test_install COMMAND test_install)

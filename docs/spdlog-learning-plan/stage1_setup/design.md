# 阶段 1 设计文档: 安装与环境搭建

## 环境需求

### 编译器
| 编译器 | 最低版本 | 推荐版本 |
|--------|---------|---------|
| GCC | 5.0 | 11+ |
| Clang | 3.4 | 14+ |
| MSVC | 2017 | 2019+ |

### 构建工具
- CMake 3.10+

### C++ 标准
- 最低: C++11
- 推荐: C++17（获得最佳兼容性和功能支持）

## 集成方式对比

### header-only 模式（推荐）
- **原理**: 只需包含头文件，无需编译为库
- **优点**: 部署简单，无链接依赖
- **缺点**: 编译时间略长
- **适用**: 嵌入式系统、减少部署复杂度

### 编译库模式
- **原理**: 编译为静态库/动态库
- **优点**: 编译快，多项目共享
- **缺点**: 需要链接，部署复杂
- **适用**: 大型项目、多模块共享

## 构建配置

### CMakeLists.txt 要点
```cmake
cmake_minimum_required(VERSION 3.10)
project(spdlog_stage1 CXX)
set(CMAKE_CXX_STANDARD 17)

# 方式1: find_package (已安装 spdlog)
find_package(spdlog REQUIRED)

# 方式2: add_subdirectory (源码集成)
add_subdirectory(/path/to/spdlog)

# 方式3: FetchContent (自动下载)
include(FetchContent)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.17.0)
FetchContent_MakeAvailable(spdlog)
```

## 决策记录
- 选择 header-only 模式集成（减少部署复杂度）
- 使用 C++17 标准编译
- 使用 FetchContent 方式获取 spdlog（便于版本管理）

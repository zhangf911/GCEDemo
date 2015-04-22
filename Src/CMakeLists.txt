#cmake工具所需版本号
cmake_minimum_required(VERSION 3.0)

#项目名称
project(Conqueror)

#设置工程版本号
set (Conqueror_VERSION_MAJOR 0)
set (Conqueror_VERSION_MINOR 1)

#配置一个头文件，通过它向源代码中传递一些CMake设置
configure_file (
"${PROJECT_SOURCE_DIR}/ConquerorConfig.h.in"
"${PROJECT_BINARY_DIR}/ConquerorConfig.h"
)

# 将二进制文件树添加到包含文件的搜索路径中，这样我们可以找到ConquerorConfig.h
include_directories("${PROJECT_BINARY_DIR}")

#工程安装目录
set(CMAKE_INSTALL_PREFIX E:/Project/Conqueror/Code/Server)

#添加子工程目录
add_subdirectory(Common)
add_subdirectory(GateServer)
add_subdirectory(GameServer)
add_subdirectory(Library)
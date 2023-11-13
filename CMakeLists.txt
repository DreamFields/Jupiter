cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(Mercury VERSION 0.1.0) 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 设置指定的C++编译器版本是必须的，如果不设置，或者为OFF，则指定版本不可用时，会使用上一版本
set(BUILD_SHARED_LIBS OFF) # 不构建共享库

set(MERCURY_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}") # 定义变量
set(CMAKE_INSTALL_PREFIX "${MERCURY_ROOT_DIR}/bin")
set(BINARY_ROOT_DIR "${CMAKE_INSTALL_PREFIX}/")

add_subdirectory(engine) # 添加一个子目录并构建该子目录

# 设置启动项目
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MercuryEditor)
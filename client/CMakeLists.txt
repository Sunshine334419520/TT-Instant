# 支持的最低版本
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0012 NEW)

#项目名称
project(GuangQ)
#设置SRC变量
set(SRC client.cpp)
#利用SRC生成一个可执行文件
add_executable(${PROJECT_NAME} ${SRC})

#Author: YangGuang <sunshine>
#Date:   2018-04-16T11:37:36+08:00
#Email:  guang334419520@126.com
#Filename: CMakeLists.txt
#Last modified by:   sunshine
#Last modified time: 2018-04-16T12:26:42+08:00



# 支持的最低版本
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0012 NEW)

#项目名称
project(GuangQ)
#设置SRC变量
set(SRC client.cpp)
#利用SRC生成一个可执行文件
add_executable(${PROJECT_NAME} ${SRC})
#支持C++11
add_definitions(-std=c++11)

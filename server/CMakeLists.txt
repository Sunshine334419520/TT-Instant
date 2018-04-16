@Author: YangGuang <sunshine>
@Date:   2018-04-16T11:49:46+08:00
@Email:  guang334419520@126.com
@Filename: CMakeLists.txt
@Last modified by:   sunshine
@Last modified time: 2018-04-16T18:51:42+08:00



# 支持的最低版本
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0012 NEW)

#项目名称
project(GuangQServer)
#设置SRC变量
set(SRC server.cpp)
#利用SRC生成一个可执行文件
add_executable(${PROJECT_NAME} ${SRC})

if(__linux__)
    message("linux system")
    target_link_libraries(${PROJECT_NAME} thread)
else()
    message("mac os system")
endif()

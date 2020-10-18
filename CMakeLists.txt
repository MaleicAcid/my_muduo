cmake_minimum_required(VERSION 2.5)
project(mymuduo)

#cmake => makefile make
#makefile 要重新生成
 
# mymuduo编译生成so动态库的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置调试信息 以及启动c++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

# 定义参与编译的源码文件
aux_source_directory(. SRC_LIST)

#   编译动态库
add_library(mymuduo SHARED ${SRC_LIST})
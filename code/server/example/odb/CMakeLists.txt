#1.添加cmake的版本号
cmake_minimum_required(VERSION 3.1.3)
#2.声明工程名称
project(all-test)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test3)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
#默认安装在build所在目录下的bin文件中

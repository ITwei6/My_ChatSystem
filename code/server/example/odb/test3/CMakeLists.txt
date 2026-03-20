#1.添加cmake的版本号
cmake_minimum_required(VERSION 3.1.3)
#2.声明工程名称
project(odb-test)
#3.检测并生成odb框架代码
   #3.1 添加所需要的odb映射代码文件名称 主要是student.hxx 和它的路径(默认是在同CMakeLists.txt目录下的entity)
set(odb_hxx_path ${CMAKE_CURRENT_SOURCE_DIR}/entity)
set(odb_hxx_files student.hxx)#可能会有多个hxx文件

set(odb_hxx "")
set(odb_cxx "")
set(odb_all_cxx "")#保存所有的cxx文件名称
   #3.2检测框架代码是否已经生成(主要检测在build执行目录下是否存在cxx文件)
foreach(odb_file ${odb_hxx_files})
    string(REPLACE ".hxx" "-odb.hxx" odb_hxx ${odb_file})
    string(REPLACE ".hxx" "-odb.cxx" odb_cxx ${odb_file})
    #3.3如果没有生成(判断在build目录下是否生成)，则预定义生成命令--用来在构建项目之前先生成框架代码
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}${odb_cxx})
        add_custom_command(
            PRE_BUILD #表示在所有其他步骤之前执行自定义命令
            COMMAND odb #要执行的指令名称
            ARGS -d mysql --generate-query --generate-schema --profile boost/date-time ${odb_hxx_path}/${odb_file}#要执行的指令运行参数选项
            DEPENDS ${odb_hxx_path}/${odb_file}#指定命令的依赖项
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx}#指定要生成的目标名称
            COMMENT "生成ODB框架代码文件:" ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx}
        )
    endif()
    #3.4将生成的框架源码文件名称保存起来，主要是student-odb.cxx, classes-odb.cxx因为后面编译连接时需要用到，所以全部保存起来，可能有多个
    list(APPEND odb_all_cxx ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx})
endforeach()  
#4.获取源码目录下的所有源码文件
set(src_files "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source src_files)
#5.声明目标以及依赖
add_definitions(-DMYSQL_CLIENT_VERSION=80000) 
add_executable(main ${src_files} ${odb_all_cxx})
#6.设置头文件的默认搜索路径,因为main函数需要使用hxx头文件和odb-hxx头文件，这两个头文件都放在了不同位置里
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/entity)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
#7.设置要连接的库
target_link_libraries(main -lodb-mysql -lodb -lodb-boost -lgflags)
#8.设置安装路径,当使用嵌套cmake
INSTALL(TARGETS main RUNTIME DESTINATION bin)


#将临时文件都生成在build的目录下，直接删除不影响其他程序
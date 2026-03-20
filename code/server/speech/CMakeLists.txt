#1.添加cmake的版本号
cmake_minimum_required(VERSION 3.1.3)
#2.声明工程名称
project(speech_server)
set(target "speech_server")
set(test   "speech_client")
#3.检测并生成proto框架代码
   #3.1 添加所需要的proto映射代码文件名称 主要是speech.proto 和它的路径(是在同CMakeLists.txt目录上一层的的prtocol下)
set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../proto)
set(proto_files speech.proto)#可能会有多个proto文件

set(proto_h "")
set(proto_cc "")
set(proto_all_cc "")#保存所有的c文件名称
   #3.2检测框架代码是否已经生成(主要检测在build执行目录下是否存在.pb.c文件)
foreach(proto_file ${proto_files})
    string(REPLACE ".proto" ".pb.h" proto_h ${proto_file})
    string(REPLACE ".proto" ".pb.cc" proto_cc ${proto_file})
    #3.3如果没有生成(判断在build目录下是否生成)，则预定义生成命令--用来在构建项目之前先生成框架代码
    # if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}${proto_cc})
        add_custom_command(
            PRE_BUILD #表示在所有其他步骤之前执行自定义命令
            COMMAND protoc #要执行的指令名称
            ARGS --experimental_allow_proto3_optional --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I ${proto_path} ${proto_path}/${proto_file}#要执行的指令运行参数选项
            DEPENDS ${proto_path}/${proto_file}#指定命令的依赖项
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}#指定要生成的目标名称
            COMMENT "生成proto框架代码文件:" ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
        )
    # endif()
    #3.4将生成的框架源码文件名称保存起来，主要是speech.pb.cc因为后面编译连接时需要用到，所以全部保存起来，可能有多个
    list(APPEND proto_all_cc ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})
endforeach()  
#4.获取源码目录下的所有源码文件
set(src_files "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source src_files)
#5.声明目标以及依赖
add_executable(${target} ${src_files} ${proto_all_cc})
target_link_libraries(${target} -lgflags -lspdlog -lpthread -letcd-cpp-api -lcpprest -lbrpc -lleveldb -lssl -lcrypto -lprotobuf -ldl -lcurl -lcrypto -ljsoncpp -lfmt /usr/lib/x86_64-linux-gnu/libjsoncpp.so.25)

set(test_files "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/test test_files)
#5.声明目标以及依赖
add_executable(${test} ${test_files} ${proto_all_cc})
target_link_libraries(${test} -lgflags -lspdlog -lpthread -letcd-cpp-api -lcpprest -lbrpc -lleveldb -lssl -lcrypto -lprotobuf -ldl -lcurl -lcrypto -ljsoncpp -lfmt /usr/lib/x86_64-linux-gnu/libjsoncpp.so.25)

#6.设置头文件的默认搜索路径,因为main函数需要使用.pb.h头文件和.asr_sdk,还有一些封装的头文件，这些头文件都放在了不同位置里
include_directories(${CMAKE_CURRENT_BINARY_DIR})#.pb.h文件就生成在当前build的目录下
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/include)
include_directories(/usr/include/jsoncpp)
#7.设置要连接的库
#8.设置安装路径,当使用嵌套cmake     
INSTALL(TARGETS ${target} RUNTIME DESTINATION bin)


#将临时文件都生成在build的目录下，直接删除不影响其他程序
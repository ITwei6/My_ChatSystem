# # 1. 添加cmake版本说明
# cmake_minimum_required(VERSION 3.1.3)
# # 2. 声明工程名称
# project(user_server)

# set(target "user_server")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# #-------------------------------------------生成proto框架------------------------------------------------------
# # 3. 检测并生成proto框架代码
# #   1. 添加所需的proto映射代码文件名称
# set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../proto)
# set(proto_files user.proto file.proto base.proto)
# #   2. 检测框架代码文件是否已经生成
# set(proto_hxx "")
# set(proto_cxx "")
# set(proto_all_cc "")
# foreach(proto_file ${proto_files})
# #   3. 如果没有生成，则预定义生成指令 -- 用于在构建项目之间先生成框架代码
#     string(REPLACE ".proto" ".pb.cc" proto_cc ${proto_file})
#     string(REPLACE ".proto" ".pb.h" proto_hh  ${proto_file})
#     if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}${proto_cc})
#         add_custom_command(
#             PRE_BUILD
#             COMMAND protoc
#             ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I ${proto_path} --experimental_allow_proto3_optional ${proto_path}/${proto_file}
#             DEPENDS ${proto_path}/${proto_file}
#             OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
#             COMMENT "生成Protobuf框架代码文件:" ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
#         )
#     endif()
#     list(APPEND proto_all_cc ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})
# endforeach()

# #-------------------------------------------生成ODB框架--------------------------------------------------
# #3.检测并生成odb框架代码
#    #3.1 添加所需要的odb映射代码文件名称 主要是user.hxx 和它的路径
# set(odb_hxx_path ${CMAKE_CURRENT_SOURCE_DIR}/../odb)
# set(odb_hxx_files  user.hxx)#可能会有多个hxx文件

# set(odb_hxx "")
# set(odb_cxx "")
# set(odb_all_cxx "")#保存所有的cxx文件名称
#    #3.2检测框架代码是否已经生成(主要检测在build执行目录下是否存在cxx文件)
# foreach(odb_file ${odb_hxx_files})
#     string(REPLACE ".hxx" "-odb.hxx" odb_hxx ${odb_file})
#     string(REPLACE ".hxx" "-odb.cxx" odb_cxx ${odb_file})
#     #3.3如果没有生成(判断在build目录下是否生成)，则预定义生成命令--用来在构建项目之前先生成框架代码
#     if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}${odb_cxx})
#         add_custom_command(
#             PRE_BUILD #表示在所有其他步骤之前执行自定义命令
#             COMMAND odb #要执行的指令名称
#             ARGS -d mysql --generate-query --generate-schema --profile boost/date-time ${odb_hxx_path}/${odb_file}#要执行的指令运行参数选项
#             DEPENDS ${odb_hxx_path}/${odb_file}#指定命令的依赖项
#             OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx}#指定要生成的目标名称
#             COMMENT "生成ODB框架代码文件:" ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx}
#         )
#     endif()
#     #3.4将生成的框架源码文件名称保存起来，主要是student-odb.cxx, classes-odb.cxx因为后面编译连接时需要用到，所以全部保存起来，可能有多个
#     list(APPEND odb_all_cxx ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx})
# endforeach()  

# #-------------------------------------------------------------------------------------------------------------
# # 4. 获取源码目录下的所有源码文件
# set(src_files "")
# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source src_files)
# # 5. 声明目标及依赖
# add_executable(${target} ${src_files} ${proto_all_cc} ${odb_all_cxx})
# # 7. 设置需要连接的库
# target_link_libraries(${target} -lgflags 
# -lspdlog -lfmt -lbrpc -lssl -lcrypto -L/usr/local/lib -lcpr -L/usr/local/lib -lelasticlient
# -lprotobuf -lleveldb -letcd-cpp-api -lodb-mysql -lodb -lodb-boost -lhiredis -lredis++
# -lcpprest -lcurl /usr/lib/x86_64-linux-gnu/libjsoncpp.so.1.9.5)


# # set(test_client "file_client")
# # set(test_files "")
# # aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/test test_files)
# # add_executable(${test_client} ${test_files} ${proto_srcs})
# # target_link_libraries(${test_client} -lgtest -lgflags -lspdlog -lfmt -lbrpc -lssl -lcrypto -lprotobuf -lleveldb -letcd-cpp-api -lcpprest -lcurl /usr/lib/x86_64-linux-gnu/libjsoncpp.so.19)

# # 6. 设置头文件默认搜索路径
# include_directories(${CMAKE_CURRENT_BINARY_DIR})
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../odb)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/include)

# #8. 设置安装路径
# INSTALL(TARGETS ${target} ${test_client} RUNTIME DESTINATION bin)



# 1. 添加cmake版本说明
cmake_minimum_required(VERSION 3.1.3)
# 2. 声明工程名称
project(transmit_server)

set(target "transmit_server")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

#-------------------------------------------生成proto框架------------------------------------------------------
# 3. 检测并生成proto框架代码
#   1. 添加所需的proto映射代码文件名称
set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../proto)
set(proto_files user.proto transmit.proto base.proto)
#   2. 检测框架代码文件是否已经生成
set(proto_hxx "")
set(proto_cxx "")
set(proto_all_cc "")
foreach(proto_file ${proto_files})
#   3. 如果没有生成，则预定义生成指令 -- 用于在构建项目之间先生成框架代码
    string(REPLACE ".proto" ".pb.cc" proto_cc ${proto_file})
    string(REPLACE ".proto" ".pb.h" proto_hh  ${proto_file})
    if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})  # 修复：漏写 / 分隔符
        add_custom_command(
            PRE_BUILD
            COMMAND protoc
            ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I ${proto_path} --experimental_allow_proto3_optional ${proto_path}/${proto_file}
            DEPENDS ${proto_path}/${proto_file}
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
            COMMENT "生成Protobuf框架代码文件:" ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
        )
    endif()
    list(APPEND proto_all_cc ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})
endforeach()

#-------------------------------------------生成ODB框架--------------------------------------------------
#3.检测并生成odb框架代码
   #3.1 添加所需要的odb映射代码文件名称 主要是user.hxx 和它的路径
set(odb_hxx_path ${CMAKE_CURRENT_SOURCE_DIR}/../odb)
set(odb_hxx_files  chat_session_member.hxx)#可能会有多个hxx文件

set(odb_hxx "")
set(odb_cxx "")
set(odb_all_cxx "")#保存所有的cxx文件名称
   #3.2检测框架代码是否已经生成(主要检测在build执行目录下是否存在cxx文件)
foreach(odb_file ${odb_hxx_files})
    string(REPLACE ".hxx" "-odb.hxx" odb_hxx ${odb_file})
    string(REPLACE ".hxx" "-odb.cxx" odb_cxx ${odb_file})
    #3.3如果没有生成(判断在build目录下是否生成)，则预定义生成命令--用来在构建项目之前先生成框架代码
    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx})  # 修复：漏写 / 分隔符
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

#-------------------------------------------------------------------------------------------------------------
# 4. 获取源码目录下的所有源码文件
set(src_files "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source src_files)

# 5. 声明目标及依赖
add_executable(${target} ${src_files} ${proto_all_cc} ${odb_all_cxx})

# 6. 设置头文件默认搜索路径（调整顺序：先设置头文件，再链接库，更规范）
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../odb
    ${JSONCPP_INCLUDE_DIRS}  # 新增：添加 jsoncpp 头文件路径
)

# 7. 设置需要连接的库（核心修复：替换硬编码 jsoncpp 为 pkg-config 自动匹配）
target_link_libraries(${target} 
    -lgflags 
    -lspdlog 
    -lfmt 
    -lbrpc 
    -lssl 
    -lcrypto 
    -lprotobuf 
    -lleveldb 
    -letcd-cpp-api 
    -lodb-mysql 
    -lodb 
    -lodb-boost 
    -lcpprest 
    -lcurl 
    -lamqpcpp -lev
)
# 注释掉测试代码（避免干扰，如需启用需同步修复 jsoncpp 链接）
set(test_client "transmit_client")
set(test_files "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/test test_files)
add_executable(${test_client} ${test_files} ${proto_all_cc})
target_link_libraries(${test_client} 
    -lgtest 
    -lgflags 
    -lspdlog 
    -lfmt 
    -lbrpc 
    -lssl 
    -lcrypto 
    -lprotobuf 
    -lleveldb 
    -letcd-cpp-api 
    -lcpprest 
    -lcurl 
)

#8. 设置安装路径（修复：test_client 未定义时注释，避免报错）
INSTALL(TARGETS ${target} ${test_client} RUNTIME DESTINATION bin)
# INSTALL(TARGETS ${test_client} RUNTIME DESTINATION bin)
# 1. 添加cmake版本说明
cmake_minimum_required(VERSION 3.1.3)
# 2. 声明工程名称
project(message_server)

set(target "message_server")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
# 新增：确保链接器能找到 /usr/local/lib 下的库（如 elasticlient/cpr）
link_directories(/usr/local/lib)
# 新增：启用 pkg-config 查找系统库（解决 jsoncpp 版本问题）
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)  # 自动匹配系统 jsoncpp 版本
# 3. 检测并生成ODB框架代码
#   1. 添加所需的proto映射代码文件名称
set(proto_path ${CMAKE_CURRENT_SOURCE_DIR}/../proto)
set(proto_files base.proto user.proto file.proto message.proto)
#   2. 检测框架代码文件是否已经生成
set(proto_hxx "")
set(proto_cxx "")
set(proto_srcs "")
foreach(proto_file ${proto_files})
#   3. 如果没有生成，则预定义生成指令 -- 用于在构建项目之间先生成框架代码
    string(REPLACE ".proto" ".pb.cc" proto_cc ${proto_file})
    string(REPLACE ".proto" ".pb.h" proto_hh  ${proto_file})
    if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}${proto_cc})
        add_custom_command(
            PRE_BUILD
            COMMAND protoc
            ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} -I ${proto_path} --experimental_allow_proto3_optional ${proto_path}/${proto_file}
            DEPENDS ${proto_path}/${proto_file}
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
            COMMENT "生成Protobuf框架代码文件:" ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc}
        )
    endif()
    list(APPEND proto_srcs ${CMAKE_CURRENT_BINARY_DIR}/${proto_cc})
endforeach()

# 3. 检测并生成ODB框架代码
#   1. 添加所需的odb映射代码文件名称
set(odb_path ${CMAKE_CURRENT_SOURCE_DIR}/../odb)
set(odb_files message_store.hxx)
#   2. 检测框架代码文件是否已经生成
set(odb_hxx "")
set(odb_cxx "")
set(odb_srcs "")
foreach(odb_file ${odb_files})
#   3. 如果没有生成，则预定义生成指令 -- 用于在构建项目之间先生成框架代码
    string(REPLACE ".hxx" "-odb.hxx" odb_hxx ${odb_file})
    string(REPLACE ".hxx" "-odb.cxx" odb_cxx ${odb_file})
    if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}${odb_cxx})
        add_custom_command(
            PRE_BUILD
            COMMAND odb
            ARGS -d mysql --std c++11 --generate-query --generate-schema --profile boost/date-time ${odb_path}/${odb_file}
            DEPENDS ${odb_path}/${odb_file}
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx}
            COMMENT "生成ODB框架代码文件:" ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx}
        )
    endif()
#   4. 将所有生成的框架源码文件名称保存起来 student-odb.cxx classes-odb.cxx
    list(APPEND odb_srcs ${CMAKE_CURRENT_BINARY_DIR}/${odb_cxx})
endforeach()

# 4. 获取源码目录下的所有源码文件
set(src_files "")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source src_files)
# 5. 声明目标及依赖
add_executable(${target} ${src_files} ${proto_srcs} ${odb_srcs})
# 7. 设置需要连接的库
target_link_libraries(${target} -lgflags 
    -lspdlog -lfmt -lbrpc -lssl -lcrypto 
    -lprotobuf -lleveldb -letcd-cpp-api 
    -lcpprest -lcurl -lodb-mysql -lodb -lodb-boost
    /usr/lib/x86_64-linux-gnu/libjsoncpp.so.19
    -lcpr -lelasticlient
    -lamqpcpp -lev
    ${JSONCPP_LIBRARIES}  # 替换硬编码：自动匹配系统 jsoncpp 库
    )


set(test_client "message_client")
# set(test_files "")
# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/test test_files)
# add_executable(${test_client} ${test_files} ${proto_srcs})
# target_link_libraries(${test_client} -pthread -lgtest -lgflags -lspdlog -lfmt -lbrpc -lssl -lcrypto -lprotobuf -lleveldb -letcd-cpp-api -lcpprest -lcurl /usr/lib/x86_64-linux-gnu/libjsoncpp.so.19)

# 6. 设置头文件默认搜索路径
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../odb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third/include)
 include_directories(${JSONCPP_INCLUDE_DIRS})  # 新增：添加 jsoncpp 头文件路径
#8. 设置安装路径
INSTALL(TARGETS ${target}  RUNTIME DESTINATION bin)
#include <gflags/gflags.h>
#include<iostream>

//定义参数
DEFINE_string(ip,"127.0.0.1","这是服务器的ip地址，格式为127.0.0.1");
DEFINE_int32(port,8080,"这是服务器的端口号，格式是8080");
DEFINE_bool(debug_enable,true,"是否启动debug模式，格式是：true/false");

int main(int argc,char*argv[])
{
    //要想通过解析命令行参数来设置到定义的变量中，需要告诉可执行程序去处理解析命令行传入的参数
    google::ParseCommandLineFlags(&argc, &argv, true); 
    //上面的定义的参数名称并不是真正的全局变量，gflags内部会将名字前面统一添加FLAGS_
    std::cout<<FLAGS_ip<<std::endl;
    std::cout<<FLAGS_port<<std::endl;
    std::cout<<FLAGS_debug_enable<<std::endl;
    return 0;
}
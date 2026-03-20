
#include "logger.hpp"
#include <gflags/gflags.h>
#include<iostream>
//想通过命令行参数调整变量的内容，所以通过gflags框架捕捉命令行参数数据放入到自己定义的全局变量中
DEFINE_bool(run_mode,false,"表示程序的运行模式，默认是false调试默认，true表示发布模式");
DEFINE_string(file,"","表示发布模式下要输出的文件名称，默认调试模式下为空");
DEFINE_int32(level,0,"表示发布模式下日志器输出的等级，默认调试模式下为0");

int main(int argc,char*argv[])
{
    //首先初始化gflags框架，告诉它要捕捉命令行中的参数数据
    google::ParseCommandLineFlags(&argc, &argv, true);
    //初始化spdlog日志器；
    init_logger(FLAGS_run_mode,FLAGS_file,FLAGS_level); 
    //日志输出
    TRACE_LOG("你好呀 {}","陶恩威");
    DEBUG_LOG("你好呀 {}","陶恩威");
    INFO_LOG("你好呀 {}","陶恩威");
    ERROR_LOG("你好呀 {}","陶恩威");
    FATAL_LOG("你好呀 {}","陶恩威");
    return 0;
}
#pragma once
//对日志器进行二次封装
//1.因为原来的日志格式中没有文件名和行号，所以要进行封装
//2.为了便于操作，通过命令行参数来决定创建日志器输出在哪里：
/*如果是debug模式则输出到标准输出中.如果是发布模式则输出到文件中*/
#include<spdlog/spdlog.h>
#include<spdlog/sinks/stdout_color_sinks.h>
#include<spdlog/sinks/basic_file_sink.h>
#include<spdlog/async.h>
//初始化日志器，有三个参数
//1.mode 表示运行的模式，true代表是发布模式，false代表是dubug模式
//2.file 表示如果是发布模式，日志输出的文件名叫什么，在debug模式下为空
//3.level 发布模式下，输出的日志等级是什么,在debug模式下为0.
namespace tew_im
{
std::shared_ptr<spdlog::logger> default_logger;
void init_logger(bool mode,const std::string&file,uint32_t level)
{
    if(mode==false)
    {
        //调试debug模式下，则创建标准输出日志器，日志器的等级最低为0,刷新策略也是最低
        default_logger=spdlog::stdout_color_mt("defaut-logger");
        default_logger->set_level(spdlog::level::level_enum::trace);
        default_logger->flush_on(spdlog::level::level_enum::trace);
    }
    else
    {
        //发布模式下，则创建文件输出日志器，日志器的等级为level，刷新策略也是level
        default_logger=spdlog::basic_logger_mt("file-logger",file);
        default_logger->set_level((spdlog::level::level_enum)level);
        default_logger->flush_on((spdlog::level::level_enum)level);
    }
    //创建完日志器之后，就要设置日志器的输出格式
    default_logger->set_pattern("[%H:%M:%S][%t][%n][%-8l]%v");
}

//日志器初始化完后，就要对日志器的输出进行封装，因为日志器的输出格式中没有文件名和行号，利用宏定义来修改
#define TRACE_LOG(format,...) tew_im::default_logger->trace(std::string("[{}:{}]")+format,__FILE__,__LINE__,##__VA_ARGS__);
#define DEBUG_LOG(format,...) tew_im::default_logger->debug(std::string("[{}:{}]")+format,__FILE__,__LINE__,##__VA_ARGS__);
#define INFO_LOG(format,...) tew_im::default_logger->info(std::string("[{}:{}]")+format,__FILE__,__LINE__,##__VA_ARGS__);
#define WARN_LOG(format,...) tew_im::default_logger->warn(std::string("[{}:{}]")+format,__FILE__,__LINE__,##__VA_ARGS__);
#define ERROR_LOG(format,...) tew_im::default_logger->error(std::string("[{}:{}]")+format,__FILE__,__LINE__,##__VA_ARGS__);
#define FATAL_LOG(format,...) tew_im::default_logger->critical(std::string("[{}:{}]")+format,__FILE__,__LINE__,##__VA_ARGS__);
}
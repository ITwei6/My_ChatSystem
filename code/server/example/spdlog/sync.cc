#include<spdlog/spdlog.h>
#include<spdlog/sinks/stdout_color_sinks.h>
#include<spdlog/sinks/basic_file_sink.h>
#include<iostream>

int main()
{
    //设置全局的刷新策略，每秒刷新一次
    spdlog::flush_every(std::chrono::seconds(1));
    //遇到debug以上级别的日志立刻刷新
    spdlog::flush_on(spdlog::level::level_enum::debug);
    //设置全局的日志输出等级
    spdlog::set_level(spdlog::level::level_enum::debug);

    //创建同步日志器（模板默认是同步工厂，不需要写，标准输出，输出到显示器上）
    //auto logger=spdlog::stdout_color_mt("defaut-logger");
    //创建同步日志器（文件输出，输出到文件上）
    auto logger=spdlog::basic_logger_mt("file-logger","sync_file.log");
    //设置日志器的刷新策略，以及输出等级，但是已经有了全局的就可以不用局部的
    // logger->flush_on(spdlog::level::level_enum::debug);
    // logger->set_level(spdlog::level::level_enum::debug);
    
    //设置日志的输出格式、
    logger->set_pattern("[%H:%M:%S][%t][%n][%-8l] %v");
    //进行简单的日志输出
    logger->trace("你好！{}","陶恩威");
    logger->debug("你好！{}","陶恩威");
    logger->info("你好！{}","陶恩威");
    logger->warn("你好！{}","陶恩威");
    logger->error("你好！{}","陶恩威");
    logger->critical("你好！{}","陶恩威");
    std::cout<<"日志输出完毕\n";
    return 0;
}

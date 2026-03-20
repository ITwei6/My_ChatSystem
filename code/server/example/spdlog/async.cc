#include<spdlog/spdlog.h>
#include<spdlog/sinks/stdout_color_sinks.h>
#include<spdlog/sinks/basic_file_sink.h>
#include<spdlog/async.h>
#include<iostream>
//异步日志器跟同步日志器的使用没有区别，只不过在创建时，需要使用异步工厂日志器模板
int main()
{
    //设置全局的刷新策略，每秒刷新一次
    spdlog::flush_every(std::chrono::seconds(1));
    //遇到debug以上级别的日志立刻刷新
    spdlog::flush_on(spdlog::level::level_enum::debug);
    //设置全局的日志输出等级
    spdlog::set_level(spdlog::level::level_enum::debug);

    //创建异步日志器（模板默认是同步工厂，创建异步就需要指定异步工厂，标准输出，输出到显示器上），会将后面的先输出
    //而原本的日志输出后输出，因为日志先放入了内存中，由线程池输出
    //auto logger=spdlog::stdout_color_mt<spdlog::async_factory>("defaut-logger");
    //异步日志器，文件输出
    auto logger=spdlog::basic_logger_mt<spdlog::async_factory>("file-logger","async_file.log");
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

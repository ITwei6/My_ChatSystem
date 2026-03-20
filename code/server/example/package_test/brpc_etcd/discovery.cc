/*etcd发现方实例 */
// #include"../common/etcd.hpp"
// #include<gflags/gflags.h>
// #include<thread>

// //想通过命令行参数调整变量的内容，所以通过gflags框架捕捉命令行参数数据放入到自己定义的全局变量中
// DEFINE_bool(run_mode,false,"表示程序的运行模式，默认是false调试默认，true表示发布模式");
// DEFINE_string(file,"","表示发布模式下要输出的文件名称，默认调试模式下为空");
// DEFINE_int32(level,0,"表示发布模式下日志器输出的等级，默认调试模式下为0");

// DEFINE_string(host,"http://127.0.0.1:2379","服务注册中心的地址");
// DEFINE_string(basedir,"/server","要注册服务的目录，即服务要监控的目录");
// DEFINE_string(instance_name,"/friend/instance","要注册的服务目录下的对应的服务实例名称");

// void online(const std::string&server,const std::string&host_ip)
// {
//     DEBUG_LOG("上线服务：{}-{}",server,host_ip);
// }
// void offline(const std::string&server,const std::string&host_ip)
// {
//     DEBUG_LOG("下线服务：{}-{}",server,host_ip);
// }
// int main(int argc,char*argv[])
// {
//     //首先初始化gflags框架，告诉它要捕捉命令行中的参数数据
//     google::ParseCommandLineFlags(&argc, &argv, true);
//     //初始化spdlog日志器；
//     init_logger(FLAGS_run_mode,FLAGS_file,FLAGS_level);
//     std::shared_ptr<Discovery> dclient=std::make_shared<Discovery>(FLAGS_host,online,offline);
//     dclient->discovery(FLAGS_basedir);
//     return 0;
// }

/*etcd+brpc改造后 */
#include "../common/etcd.hpp"
#include <gflags/gflags.h>
#include <thread>
#include <brpc/channel.h>
#include "main.pb.h"
#include "../common/channel.hpp"
// 想通过命令行参数调整变量的内容，所以通过gflags框架捕捉命令行参数数据放入到自己定义的全局变量中
DEFINE_bool(run_mode, false, "表示程序的运行模式，默认是false调试默认，true表示发布模式");
DEFINE_string(file, "", "表示发布模式下要输出的文件名称，默认调试模式下为空");
DEFINE_int32(level, 0, "表示发布模式下日志器输出的等级，默认调试模式下为0");

DEFINE_string(host, "http://127.0.0.1:2379", "服务注册中心的地址");
DEFINE_string(basedir, "/server", "即服务要监控的目录");
DEFINE_string(call_service, "/server/echo", "要查找的服务名称");

// 1.首先构造一个Rpc所有服务管理对象 2.构造一个服务发现对象 3.通过Rpc信道管理对象，获取提供echo服务的信道 4.发起echorpc调用
int main(int argc, char *argv[])
{
    // 首先初始化gflags框架，告诉它要捕捉命令行中的参数数据
    google::ParseCommandLineFlags(&argc, &argv, true);
    // 初始化spdlog日志器；
    init_logger(FLAGS_run_mode, FLAGS_file, FLAGS_level);

    // 1.首先构造一个Rpc所有服务管理对象
    auto service_manager = std::make_shared<ServiceManager>();
    service_manager->Declared(FLAGS_call_service); // 提前声明关心什么服务
    auto put_cb = std::bind(&ServiceManager::ServiceOnline, service_manager.get(), std::placeholders::_1, std::placeholders::_2);
    auto delete_cb = std::bind(&ServiceManager::ServiceOffline, service_manager.get(), std::placeholders::_1, std::placeholders::_2);
    // 2.构造一个服务发现对象
    Discovery::ptr dclient = std::make_shared<Discovery>(FLAGS_host, put_cb, delete_cb);
    dclient->discovery(FLAGS_basedir);

    while (1)
    {
        // 4. 发起EchoRpc调用
        auto channel = service_manager->Choose(FLAGS_call_service);
        if (!channel)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return -1;
        }
        // 构建stub类对象，用于进行rpc调用
        example::EchoService_Stub stub(channel.get());
        // 进行rpc调用
        brpc::Controller *controler = new brpc::Controller();
        example::EchoRequest req;
        req.set_message("hello tew");
        example::EchoResponse *rsp = new example::EchoResponse();
        // 同步模式下使用这个
        stub.echo(controler, &req, rsp, nullptr);
        if (controler->Failed() == true)
        {
            std::cout << "rpc调用失败" << controler->ErrorText() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            delete controler;
            delete rsp;
            continue;
        }
        // 调用成功，获取响应
        std::cout << "调用rpc成功，收到响应：" << rsp->message() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        delete controler;
        delete rsp;
    }

    return 0;
}
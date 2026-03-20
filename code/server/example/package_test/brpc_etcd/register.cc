
/*etcd注册样例*/
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
// DEFINE_string(host_id,"127.0.0.1:8888","服务对应的主机地址");
// int main(int argc,char*argv[])
// {
//     //首先初始化gflags框架，告诉它要捕捉命令行中的参数数据
//     google::ParseCommandLineFlags(&argc, &argv, true);
//     //初始化spdlog日志器；
//     init_logger(FLAGS_run_mode,FLAGS_file,FLAGS_level); 
//     std::shared_ptr<Registry> rclient=std::make_shared<Registry>(FLAGS_host);
//     rclient->registry(FLAGS_basedir+FLAGS_instance_name,FLAGS_host_id);
//     std::this_thread::sleep_for(std::chrono::seconds(600));


//     return 0;    
// }

/*brpc+etcd改造后*/
#include"../common/etcd.hpp"
#include<gflags/gflags.h>
#include<thread>
#include<brpc/server.h>
#include"main.pb.h"
#include<butil/logging.h>

//想通过命令行参数调整变量的内容，所以通过gflags框架捕捉命令行参数数据放入到自己定义的全局变量中
DEFINE_bool(run_mode,false,"表示程序的运行模式，默认是false调试默认，true表示发布模式");
DEFINE_string(file,"","表示发布模式下要输出的文件名称，默认调试模式下为空");
DEFINE_int32(level,0,"表示发布模式下日志器输出的等级，默认调试模式下为0");

DEFINE_string(host,"http://127.0.0.1:2379","服务注册中心的地址");
DEFINE_string(basedir,"/server","要注册服务的目录，即服务要监控的目录");
DEFINE_string(instance_name,"/echo/instance","要注册的服务目录下的对应的服务实例名称");
DEFINE_string(host_id,"127.0.0.1:8888","提供服务对应的主机地址");
DEFINE_int32(listend_host_id,8888,"rpc服务器监听的端口");
//1.构造echo服务  2.搭建rpc服务器  3.启动rpc服务器 4.注册服务
class EchoSeriveImpl:public example::EchoService
{
    public:
    //重写虚函数echo，也就是实现业务
    void echo(google::protobuf::RpcController* controller,
                       const ::example::EchoRequest* request,
                       ::example::EchoResponse* response,
                       ::google::protobuf::Closure* done)
    {
        brpc::ClosureGuard rpc_guard(done);
        //获取请求数据，进行业务处理，然后放入响应中
        std::cout<<"收到数据,进行业务处理："<<request->message()<<std::endl;
        //这里没有业务处理,直接回显给客户端
        std::string str=request->message()+"--这里处理完后的响应";
        response->set_message(str);
        //本来用户需要显示调用done。但是我们可以通过一个智能对象rpc_guard来管理done，最后都会调用
    }

};
int main(int argc,char*argv[])
{
    //首先初始化gflags框架，告诉它要捕捉命令行中的参数数据
    google::ParseCommandLineFlags(&argc, &argv, true);
    //初始化spdlog日志器；
    init_logger(FLAGS_run_mode,FLAGS_file,FLAGS_level); 

    //关闭服务端的输出日志
    logging::LoggingSettings settings;
    settings.logging_dest=logging::LoggingDestination::LOG_TO_NONE;
    logging::InitLogging(settings);
    
    //2.创建rpc服务器对象
    brpc::Server server;
    //3.向rpc服务器对象中新增serverImpl服务,以及对应的配置选项
    EchoSeriveImpl echo_service;
    int ret=server.AddService(&echo_service,brpc::ServiceOwnership::SERVER_DOESNT_OWN_SERVICE);
    if(ret==-1)
    {
        std::cout<<"添加rpc服务失败"<<std::endl;
        return -1;
    }
    //4.启动服务器,设置对应的端口及连接配置选项
    brpc::ServerOptions opt;
    opt.idle_timeout_sec=-1;//连接超时后断开，-1表示关闭这个功能
    opt.num_threads=1;//服务器中进行io线程的数量
    ret=server.Start(FLAGS_listend_host_id,&opt);
    if(ret==-1)
    {
        std::cout<<"启动rpc服务器失败"<<std::endl;
        return -1;
    }
    //注册服务
    Registry::ptr rclient=std::make_shared<Registry>(FLAGS_host);
    rclient->registry(FLAGS_basedir+FLAGS_instance_name,FLAGS_host_id);
    /*存在问题：注册时是注册一个实例：/service/echo/instance1-127.0.0.1:7777
               而服务发现时，是通过服务名称来获取的 /service/echo 来获取服务的
               */
    //修改等待运行结束,休眠直到 ctrl+c 按下，或者 stop 和 join 服务器 
    server.RunUntilAskedToQuit();
    return 0;    
}
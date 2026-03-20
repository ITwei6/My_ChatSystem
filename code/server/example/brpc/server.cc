#include<brpc/server.h>
#include"main.pb.h"
#include<butil/logging.h>

//1.首先继承EchoServer创建一个子类服务ServerImpl，然后重写rpc方法echo，实现业务
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

int main()
{
    //关闭服务端的输出日志
    logging::LoggingSettings settings;
    settings.logging_dest=logging::LoggingDestination::LOG_TO_NONE;
    logging::InitLogging(settings);
    //2.创建服务器对象
    brpc::Server server;
    //3.向服务器对象中新增serverImpl服务,以及对应的配置选项
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
    ret=server.Start(8080,&opt);
    if(ret==-1)
    {
        std::cout<<"启动rpc服务器失败"<<std::endl;
        return -1;
    }
    //修改等待运行结束,休眠直到 ctrl+c 按下，或者 stop 和 join 服务器 
    server.RunUntilAskedToQuit();
    return 0;
}




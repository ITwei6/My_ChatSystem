
#include<brpc/channel.h>
#include"main.pb.h"
#include<thread>
void callback(brpc::Controller* controler, ::example::EchoResponse* response)
{
    //异步模式下，如果在调用rpc结束后请求还没有处理完毕的，主线程不会阻塞等待，而是继续往下执行任务，而当请求处理完毕，返回响应时
    //由设置的回调函数来处理
    if(controler->Failed()==true)
    {
        std::cout<<"rpc调用失败"<<controler->ErrorText()<<std::endl;
        return;
    }
    //调用成功，获取响应
    std::cout<<"调用rpc成功，收到响应："<<response->message()<<std::endl;
    delete controler;
    delete response;
}
// int main()
// {
//     //1.构建channel网络通信信道，连接服务器
//     brpc::Channel channel;
//     //信道连接服务器，并配置一些连接选项
//     brpc::ChannelOptions opt;
//     //请求连接超时时间 
//     opt.connect_timeout_ms=-1;//-1表示一直等
//     //rpc 请求超时时间
//     opt.timeout_ms=-1;
//     //最大重试次数 
//     opt.max_retry=3;
//     //序列化协议类型
//     opt.protocol = "baidu_std"; 
//     int ret=channel.Init("127.0.0.1:8080",&opt);
//     if(ret==-1)
//     {
//         std::cout<<"创建信道失败"<<std::endl;
//         return -1;
//     }
//     //2.构建stub类对象，用于进行rpc调用
//     example::EchoService_Stub stub(&channel);
//     //3.进行rpc调用
    
//     /*先构建一个请求，然后调用rpc接口，将请求通过rpc发送给服务器，服务器进行业务处理返回响应
//     rpc接口有四个参数：
//     ①RpcController* controller,用来判断请求是否发送成功的上下文,即判断rpc是否调用成功
//     ②EchoRequest* request,   rpc请求
//     ③EchoResponse* response, rpc响应
//     ④protobuf::Closure* done 用于异步rpc调用时，程序已结束，但响应还没有得到情况下的回调函数，用于处未处理的响应*/
//     brpc::Controller*controler=new brpc::Controller();
    
//     example::EchoRequest req;
//     req.set_message("hello tew");

//     example::EchoResponse*rsp=new  example::EchoResponse();
//     //同步模式下使用这个
//     // stub.echo(controler,&req,rsp,nullptr);
//     //异步模式下使用这个
//     auto closure=google::protobuf::NewCallback(callback,controler,rsp);
//     stub.echo(controler,&req,rsp,closure);
//     if(controler->Failed()==true)
//     {
//         std::cout<<"rpc调用失败"<<controler->ErrorText()<<std::endl;
//         return -1;
//     }
//     //调用成功，获取响应
//     std::cout<<"调用rpc成功，收到响应："<<rsp->message()<<std::endl;
//     delete controler;
//     delete rsp;
//     return 0;
// }
//异步
int main()
{
    //1.构建channel网络通信信道，连接服务器
    brpc::Channel channel;
    //信道连接服务器，并配置一些连接选项
    brpc::ChannelOptions opt;
    //请求连接超时时间 
    opt.connect_timeout_ms=-1;//-1表示一直等
    //rpc 请求超时时间
    opt.timeout_ms=-1;
    //最大重试次数 
    opt.max_retry=3;
    //序列化协议类型
    opt.protocol = "baidu_std"; 
    int ret=channel.Init("127.0.0.1:8080",&opt);
    if(ret==-1)
    {
        std::cout<<"创建信道失败"<<std::endl;
        return -1;
    }
    //2.构建stub类对象，用于进行rpc调用
    example::EchoService_Stub stub(&channel);
    //3.进行rpc调用
    brpc::Controller*controler=new brpc::Controller();
    example::EchoRequest req;
    req.set_message("hello tew");
    example::EchoResponse*rsp=new  example::EchoResponse();
  
    //异步模式下使用这个
    auto closure=google::protobuf::NewCallback(callback,controler,rsp);
    stub.echo(controler,&req,rsp,closure);
    //这是一个非阻塞的接口，它不管调用成功与否，都会立刻返回，等到拿到响应，就会调用回调函数
    std::cout<<"异步调用结束\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 0;
}
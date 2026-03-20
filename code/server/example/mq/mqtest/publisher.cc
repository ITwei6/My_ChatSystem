
#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>

int main()
{
    //1.实例化底层网络通信框架的IO事件监控句柄
    auto *loop =EV_DEFAULT;
    //2.实例化libEvHandler句柄，将AMQP框架与事件监控关联起来
    AMQP::LibEvHandler handler(loop);
    //3.实例化连接对象
    AMQP::Address address("amqp://root:123456@127.0.0.1:5672/");
    AMQP::TcpConnection connection(&handler,address);
    //4.实例化信道对象
    AMQP::TcpChannel channel(&connection);
    //5.声明交换机
    channel.declareExchange("test_exchange",AMQP::ExchangeType::direct)
    .onError([](const char*message){
        std::cout<<"声明交换机失败:"<<message<<std::endl;
        exit(0);
    })
    .onSuccess([](){
        std::cout<<"test_exchange交换机创建成功"<<std::endl;
    });//不允许拷贝构造，直接使用返回值
    //6.声明队列
    channel.declareQueue("test_queue")
    .onError([](const char*message){
        std::cout<<"声明队列失败:"<<message<<std::endl;
        exit(0);
    })
    .onSuccess([](){
        std::cout<<"test_queue队列创建成功"<<std::endl;
    });
    //7.将交换机和队列进行绑定
    channel.bindQueue("test_exchange","test_queue","test_queue_key")
    .onError([](const char*message){
        std::cout<<"test_exchange-test_queue绑定失败:"<<message<<std::endl;
        exit(0);
    })
    .onSuccess([](){
        std::cout<<"test_exchange-test_queue绑定成功"<<std::endl;
    });
    //8.向交换机发布消息
    for(int i=0;i<10;i++)
    {
        std::string msg="hello tew"+std::to_string(i);
        bool ret=channel.publish("test_exchange","test_queue_key",msg);
        if(ret==false){
            std::cout<<"publish失败"<<std::endl;
        }
    }
    //9.启动底层网络通信框架--IO监控
    ev_run(loop,0);
    return 0;
}
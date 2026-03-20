
#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>


//收到消息后就要进行应答，要通过channel进行发送应答，所以还需要一个channel对象参数
void MessageCb(AMQP::TcpChannel*channel,const AMQP::Message &message,uint64_t deliveryTag,bool redelivered)
{
    std::string msg;
    msg.assign(message.body(),message.bodySize());
    std::cout<<msg<<std::endl;
    //对消息应答
    channel->ack(deliveryTag);
}
int main()
{
    //1.实例化底层网络通信框架的IO事件监控句柄
    auto *loop =EV_DEFAULT;
    //2.实例化libEvHandler句柄，将AMQP框架与事件监控关联起来
    AMQP::LibEvHandler handler(loop);
    //3.实例化连接对象
    AMQP::Address address("amqp://root:123456@127.0.0.1:5672/");//网页端口是15672，而默认server的端口是5672
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
    });
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
    //8.订阅队列的消息
    auto callback=std::bind(MessageCb,&channel,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
    channel.consume("test_queue","comsuer_tag")
    .onReceived(callback)
    .onError([](const char*message){
        std::cout<<"订阅队列失败:"<<message<<std::endl;
        exit(0);
    });
    //9.启动底层网络通信框架--IO监控
    ev_run(loop,0);
    return 0;
}
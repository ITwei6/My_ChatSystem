
#pragma
#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#include <thread>
#include "logger.hpp"
// 封装rabbitmq，只提供三个接口：1.声明交换机与队列并绑定在一起 2.向指定交换机发布消息 3.订阅指定队列
namespace tew_im
{
    class MQClient
    {
    public:
        using callback = std::function<void(const char *, size_t)>;
        using ptr = std::shared_ptr<MQClient>;
        // 构建函数就是将前置工作都做完，比如与mq服务器建立起连接
        // 所以当MQClient对象实例完处出来后，就已经连接好服务器比较启动启动事件监控了
        MQClient(const std::string &user, const std::string &paswd, const std::string &host)
        {
            // 1.实例化底层网络通信框架的IO事件监控句柄
            _loop = EV_DEFAULT;
            // 2.实例化libEvHandler句柄，将AMQP框架与事件监控关联起来
            _handler = std::make_unique<AMQP::LibEvHandler>(_loop);
            // 3.实例化连接对象
            std::string str = "amqp://" + user + ":" + paswd + "@" + host;
            AMQP::Address address(str);
            _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), address);
            // 4.实例化信道对象
            _channel = std::make_unique<AMQP::TcpChannel>(_connection.get());
            // 在线程中启动事件监控
            _loop_thread = std::thread([this]()
                                       { ev_run(_loop); });
        }
        // 1.创建交换机与队列并绑定
        void delcarcomponent(const std::string &exchage, const std::string &queue, const std::string &routing_key = "routing_key",
                             AMQP::ExchangeType type = AMQP::ExchangeType::direct)
        {
            // 5.声明交换机
            _channel->declareExchange(exchage, type)
                .onError([](const char *message)
                         {
                ERROR_LOG("声明交换机失败:{}",message);
                exit(0); })
                .onSuccess([exchage]()
                           { DEBUG_LOG("{}交换机创建成功", exchage); }); // 不允许拷贝构造，直接使用返回值
            // 6.声明队列
            _channel->declareQueue(queue)
                .onError([](const char *message)
                         {
                ERROR_LOG("声明队列失败:{}",message);
                exit(0); })
                .onSuccess([queue]()
                           { DEBUG_LOG("{}队列创建成功", queue); });
            // 7.将交换机和队列进行绑定
            _channel->bindQueue(exchage, queue, routing_key)
                .onError([exchage, queue](const char *message)
                         {
                ERROR_LOG("{}-{}绑定失败:{}",exchage,queue,message);
                exit(0); })
                .onSuccess([exchage, queue, routing_key]()
                           { DEBUG_LOG("{}-{}-{}绑定成功", exchage, queue, routing_key); });
        }
        // 2.向指定交换机发布消息
        bool publish(const std::string &exchange, const std::string &msg, const std::string routing_key = "routing_key")
        {
            bool ret = _channel->publish(exchange, routing_key, msg);
            if (ret == false)
            {
                ERROR_LOG("发布消息失败");
                return false;
            }
            DEBUG_LOG("往{}-{}发布消息成功", exchange, routing_key);
            return true;
        }
        // 3.向指定队列订阅消息
        void consume(const std::string &queue, const callback &cb)
        {
            DEBUG_LOG("开始订阅{}队列消息", queue);
            _channel->consume(queue)
                .onReceived([this, cb](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
                            {
            cb(message.body(),message.bodySize());
            _channel->ack(deliveryTag); })
                .onError([](const char *message)
                         {
        ERROR_LOG("订阅队列消息失败:{}",message);
        exit(0); });
            DEBUG_LOG("订阅{}队列消息成功", queue);
        }
        // 析构函数，要将线程退出等待，但是主线程它不能直接退出线程， 因为线程中还在执行者事件监控，
        // 这个属于异步操作了，应该让该线程自己在完成所有事件处理后去关闭监控，而不是由主线程去关闭。
        // 所以就需要异步操作，将异步事件发送给线程执行
        ~MQClient()
        {
            // 1.初始化异步事件结构，并设置回调函数
            ev_async_init(&_async_watcher, watcher_callback);
            // 2.启动事件监控循环中的异步任务处理
            ev_async_start(_loop, &_async_watcher);
            // 3.发送当前异步事件到异步线程中执行
            ev_async_send(_loop, &_async_watcher);
            // 这些操作在内部会自动将loop给释放掉
            _loop_thread.join();
            _loop = nullptr;
        }

    private:
        static void watcher_callback(struct ev_loop *loop, ev_async *watcher, int32_t revents)
        {
            ev_break(loop, EVBREAK_ALL);
        }
        struct ev_async _async_watcher; // 用来执行异步操作的
        struct ev_loop *_loop;
        // 因为这些变量如果初始化都需要带参初始化，需要放入初始化列表，不想放入初始化列表，使用指针形式构建
        std::unique_ptr<AMQP::LibEvHandler> _handler;
        std::unique_ptr<AMQP::TcpConnection> _connection;
        std::unique_ptr<AMQP::TcpChannel> _channel;
        std::thread _loop_thread; // 用来执行事件监控的,主执行流要执行其他任务，所以监控事件需要其他线程执行
    };

}
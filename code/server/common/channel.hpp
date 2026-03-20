#pragma once
#include <brpc/channel.h>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "logger.hpp"
#include <string>
// 单个服务管理信道类
namespace tew_im
{
class ServiceChannel
{
public:
    using ptr = std::shared_ptr<ServiceChannel>;
    using ChannelPtr = std::shared_ptr<brpc::Channel>;

private:
    std::mutex _mutex;                                  // 访问顺序表与映射表时都需要加锁访问
    uint32_t _index;                                    // 当前RR轮转的下标
    std::string _service_name;                          // 当前服务的名称
    std::vector<ChannelPtr> _channels;                  // 当前服务的所有信道集合
    std::unordered_map<std::string, ChannelPtr> _hosts; // 主机host与对应信道的映射表
public:
    // 构造函数，用服务名称构建服务管理对象
    ServiceChannel(const std::string &name) : _index(0), _service_name(name)
    {
    }
    // 该服务新增了一个结点，则用apeend将信道添加进来
    void append(const std::string &host)
    {
        // 1.首先先构造一个channel对象，用来连接主机结点服务器
        auto channel = std::make_shared<brpc::Channel>();
        // 信道连接服务器，并配置一些连接选项
        brpc::ChannelOptions opt;
        // 请求连接超时时间
        opt.connect_timeout_ms = -1; //-1表示一直等
        // rpc 请求超时时间
        opt.timeout_ms = -1;
        // 最大重试次数
        opt.max_retry = 3;
        // 序列化协议类型
        opt.protocol = "baidu_std";
        //这个信道的连接的端口就是rpc服务器监听的端口
        int ret = channel->Init(host.c_str(), &opt);
        if (ret == -1)
        {
            ERROR_LOG("新增{}-{}结点时，信道初始化失败", _service_name, host);
            return;
        }
        std::unique_lock<std::mutex> lock(_mutex); // 加锁
        // 2.将信道添加到管理表中，并生成host与channel对应的映射表，用于删除
        _channels.push_back(channel);
        _hosts.insert(std::make_pair(host, channel));
    }

    // 该服务删除了一个结点，则用remove从管理表中移除出去
    void Remove(const std::string &host)
    {
        std::unique_lock<std::mutex> lock(_mutex); // 加锁
        // 首先先从hosts映射表中找到对应的channel对象
        auto it = _hosts.find(host);
        if (it == _hosts.end())
        {
            ERROR_LOG("移除{}-{}结点时，无该结点信息", _service_name, host);
            return;
        }
        // 找到就从channels表中删除该信道
        for (auto i = _channels.begin(); i != _channels.end(); i++)
        {
            if (*i == it->second)
            {
                _channels.erase(i);
                break;
            }
        }
        // 最后将hosts该信道信息也删除
        _hosts.erase(it);
    }
    // 获取该服务的信道，则使用Choose()，RR轮转返回一个信道
    ChannelPtr Choose()
    {
        if (_channels.size() == 0)
        {
            ERROR_LOG("当前没有{}服务的结点", _service_name);
            return ChannelPtr();
        }
        std::unique_lock<std::mutex> lock(_mutex); // 加锁
        uint32_t idx = (_index++) % _channels.size();
        return _channels[idx];
    }
};

// 所有服务的服务对象管理类
class ServiceManager
{
public:
    using ptr = std::shared_ptr<ServiceManager>;
    using ChannelPtr = std::shared_ptr<brpc::Channel>;

private:
    std::mutex _mutex; // 锁
    // 服务名称与对应的服务管理对象映射表
    std::unordered_map<std::string, ServiceChannel::ptr> _services;
    // 这里只存储关心的服务，不关心的服务不管它
    std::unordered_set<std::string> _attention_service;
private:
    std::string GetServiceName(const std::string service_instance)
    {
        // /service/echo/instance
        int pos=service_instance.find_last_of('/');//找到最后一个字符/的位置
        return service_instance.substr(0,pos);
    } 
public: 
    ServiceManager()
    {}
    // 先声明，我关注哪些服务的上下线，其他服务的上下线我不管理
    void Declared(const std::string &service_name)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _attention_service.insert(service_name);
    }
    // 服务上线时调用的回调函数
    //etcd服务器中发现该服务(/service/echo目录下)存在实例，就说明上线，就自动调用该回调函数，并将服务名与主机地址传过来
    //形式是/server/echo/instance1-127.0.0.1：7777
    //    server/echo/instance2-127.0.0.1：7778
    //而关心的服务是 server/echo.所以在最开始就会退出
    void ServiceOnline(const std::string &service_instance, const std::string &host)
    {
        //这里面都是根据服务名称与服务管理对象进行映射的，不能是实例名称，所以需要将实例前面的服务名称获取
        std::string service_name=GetServiceName(service_instance);
        ServiceChannel::ptr service;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            // 先判断一下是否是关心的服务上线，如果不是则不管它
            for(auto&x:_attention_service)
            {
                DEBUG_LOG("当前关心的服务是：{}",x);
            }
            auto ait = _attention_service.find(service_name);
            if (ait == _attention_service.end())
            {
                WARN_LOG("{}服务的{}结点，上线，但是当前服务不关心", service_name, host);
                return;
            }
            // 服务上线时，先看这个服务是否是第一次上线，就看映射表中是否有该服务管理对象
            auto it = _services.find(service_name);
            if (it == _services.end())
            {
                // 说明表中没有该对象，则表明该服务是第一次上线,则需要为服务构建服务管理对象
                service = std::make_shared<ServiceChannel>(service_name);
                // 并将它们的映射关系记录下来
                _services.insert(std::make_pair(service_name, service));
            }
            else
            {
                // 说明表中有该服务对象，则表明该服务早就上线过，现在又上线了一个结点，需要将上线的结点给服务对象管理
                service = it->second;
            }
            if (!service) {
                ERROR_LOG("新增 {} 服务管理节点失败！", service_name);
                return ;
            }
        }
        service->append(host); // 这里面有自己的锁，不需要双重加锁
        DEBUG_LOG("{}服务上线新结点：{} 进行添加管理，添加到该服务的信道管理中",service_name, host);
    }
    // 服务线下时调用的回调函数
    void ServiceOffline(const std::string &service_instance, const std::string &host)
    {
        std::string service_name=GetServiceName(service_instance);
        ServiceChannel::ptr service;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            // 如果是不关心的服务下线则不处理,只处理关心的服务的上下线
            //  先判断一下是否是关心的服务上线，如果不是则不管它
            auto ait = _attention_service.find(service_name);
            if (ait == _attention_service.end())
            {
                WARN_LOG("{}服务的{}结点，下线，但是当前服务不关心", service_name, host);
                return;
            }
            auto it = _services.find(service_name);
            if (it == _services.end())
            {
                // 则说明没有找到对应的服务管理对象
                WARN_LOG("下线{}服务上的{}结点时，没有找到服务对象", service_name, host);
                return;
            }
            service=it->second;
        }
        // 找到就直接删除该服务对象上的结点
        service->Remove(host);
        DEBUG_LOG("{}服务下线新结点：{} 进行移除管理，从该服务的信道管理中移除",service_name, host);
        
    }
    // 获取指定服务名称的服务信道
    ChannelPtr Choose(const std::string &service_name)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        // 客户端根据服务名称，先获取对应服务管理对象
        auto it = _services.find(service_name);
        if (it == _services.end())
        {
            // 说明没有找到
            ERROR_LOG("客户端指定{}服务，没有找到，无该服务结点", service_name);
            return ChannelPtr();
        }
        // 找到服务对象后，就从服务对象中返回一个结点信道
        return it->second->Choose();
    }
};
}
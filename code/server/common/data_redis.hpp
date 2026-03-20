#pragma once
#include <sw/redis++/redis.h>
#include <gflags/gflags.h>
#include <memory>


// 使用redis对用户主要管理三部分：会话管理，状态管理，验证码管理
namespace tew_im
{
class RedisClientFactory
{
public:
    
    static std::shared_ptr<sw::redis::Redis> create(const std::string &host,
        const int&port,const int&db,const bool&keep_alive)
    {
        //构建连接选项，实例化redis客户端，连接服务器
        sw::redis::ConnectionOptions opt;
        opt.host=host;
        opt.port=port;
        opt.keep_alive=keep_alive;
        return std::make_shared<sw::redis::Redis>(opt);
    }
};
//会话管理：新增会话，移除会话，获取会话
class Session
{
public:
    using ptr = std::shared_ptr<Session>;
    Session(std::shared_ptr<sw::redis::Redis> client) : _client(client) {}
    void insert(const std::string&sid,const std::string&uid)
    {
        _client->set(sid,uid);
    }
    void remove(const std::string&sid)
    {
        _client->del(sid);
    }
    sw::redis::OptionalString getuid(const std::string&sid)
    {
        return _client->get(sid);
    }
private:
    std::shared_ptr<sw::redis::Redis> _client;
};

//登录状态的新增，删除，判断是否已经存在
class Status
{
public:
    using ptr = std::shared_ptr<Status>;
    Status(std::shared_ptr<sw::redis::Redis> client) : _client(client) {}
    void insert(const std::string&uid)
    {
        _client->set(uid,"");
    }
    void remove(const std::string&uid)
    {
        _client->del(uid);
    }
    bool isexists(const std::string&uid)
    {
        auto res=_client->get(uid);
        if(res)return true;
        return false;
    }
private:
    std::shared_ptr<sw::redis::Redis> _client;
};
//验证码的新增，删除，获取
class Codes
{
public:
    using ptr = std::shared_ptr<Codes>;
    Codes(std::shared_ptr<sw::redis::Redis> client) : _client(client) {}
    void insert(const std::string&cid,const std::string&code,
        const std::chrono::milliseconds&t=std::chrono::milliseconds(3000))
    {
        //验证码获取后是有时间限制的，60秒内自动销毁
        _client->set(cid,code,t);
    }
    void remove(const std::string&cid)
    {
        _client->del(cid);
    }
    sw::redis::OptionalString getcode(const std::string&cid)
    {
        return _client->get(cid);
    }
private:
    std::shared_ptr<sw::redis::Redis> _client;
};
}
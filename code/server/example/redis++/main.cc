#include <sw/redis++/redis.h>
#include <gflags/gflags.h>
#include<iostream>
#include<thread>
DEFINE_string(host,"127.0.0.1","这是redis服务器的ip地址，格式为127.0.0.1");
DEFINE_int32(port,6379,"这是redis服务器的端口号，格式是8080");
DEFINE_int32(db,0,"库的编号，格式是0");
DEFINE_bool(keep_alive,true,"是否启动长连接保活，格式是：true/false");



void print(sw::redis::Redis& client)
{
    sw::redis::OptionalString os1=client.get("用户1");
    if(os1) std::cout<<*os1<<std::endl;
    sw::redis::OptionalString os2=client.get("用户2");
    if(os2) std::cout<<*os2<<std::endl;
    sw::redis::OptionalString os3=client.get("用户3");
    if(os3) std::cout<<*os3<<std::endl;
    sw::redis::OptionalString os4=client.get("用户4");
    if(os4) std::cout<<*os4<<std::endl;
    sw::redis::OptionalString os5=client.get("用户5");
    if(os5) std::cout<<*os5<<std::endl;
}
void test_string(sw::redis::Redis& client)
{
    //新增字符串键值对数据
    client.set("用户1","陶恩威");
    client.set("用户2","陶恩豪");
    client.set("用户3","陶改");
    client.set("用户4","陶梦");
    client.set("用户5","陶西力");
    //更新字符串键值对数据
    client.set("用户1","姚子怡");
    //删除字符串键值对数据
    client.del("用户2");
    //查询键值对数据
    print(client);
}
void test_time(sw::redis::Redis& client)
{
    //设置改数据的有效时间是1秒
    client.set("用户1","陶哈哈",std::chrono::milliseconds(1000));
    print(client);
    std::cout<<"-------------休眠2s-------\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    print(client);

}
void test_list(sw::redis::Redis& client)
{
    //在列表中新增数据，右插
    client.rpush("群聊一","用户1");
    client.rpush("群聊一","用户2");
    client.rpush("群聊一","用户3");
    client.rpush("群聊一","用户4");
    client.rpush("群聊一","用户5");
    std::vector<std::string> users;
    client.lrange("群聊一",0,-1,std::back_inserter(users));
    for(auto user:users)
    {
        std::cout<<user<<std::endl;
    }

}
int main()
{
    //构建连接选项，实例化redis客户端，连接服务器
    sw::redis::ConnectionOptions opt;
    opt.host=FLAGS_host;
    opt.port=FLAGS_port;
    opt.db=FLAGS_db;
    opt.keep_alive=FLAGS_keep_alive;
    sw::redis::Redis client(opt);
    //新增字符串键值对，删除键值对，查看键值对
    //test_string(client);
    //设置键值对的有效存活时间
     //test_time(client);
    //实现对列表的操作，主要是数据的右插，左获取
    test_list(client);
}
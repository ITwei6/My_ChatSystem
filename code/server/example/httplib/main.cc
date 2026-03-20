#include "../common/httplib.h"

//搭建http服务器，简单测试

void hello(const httplib::Request&req,httplib::Response&rsp)
{
    //处理请求
    //设置响应的正文
    std::string body="hello tew";
    rsp.set_content(body,"text/plain");
}
int main()
{
    //1.实例化http服务器对象
    httplib::Server server;
    //2.注册对应的方法的对应回调函数
    server.Get("/hello",hello);
    //3.启动服务器
    server.listen("0.0.0.0",9090);
}
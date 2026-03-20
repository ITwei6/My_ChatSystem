#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
// 简单websocketpp服务器的搭建，实现回显功能

// 1.重定义服务器类型，使其实例化时比较简便
typedef websocketpp::server<websocketpp::config::asio> server_t;

void OnOpen(websocketpp::connection_hdl hdl)
{
    std::cout<<"websocket长连接建立成功\n";
}
void OnMessage(server_t *server,websocketpp::connection_hdl hdl, server_t::message_ptr msg)
{
    //首先获取请求的正文,然后进行业务处理
    std::string body=msg->get_payload();
    std::cout<<"服务器收到："<<body<<std::endl;
    //将响应发送给客户端
    //获取到对应连接
    auto conn=server->get_con_from_hdl(hdl);
    conn->send("响应："+body+"--hello",websocketpp::frame::opcode::text);
}
void OnClose(websocketpp::connection_hdl hdl)
{
    std::cout<<"websocket长连接断开\n";
}
int main()
{
    // 2.实例化服务器对象server
    server_t server;
    // 3.设置日志输出等级
    server.set_access_channels(websocketpp::log::alevel::all);
    // 4.初始化asio网络框架
    server.init_asio();
    // 5.设置连接建立成功/消息/连接关闭/等回调函数
    server.set_open_handler(OnOpen);
    auto message_handler=std::bind(OnMessage,&server,std::placeholders::_1,std::placeholders::_2);
    server.set_message_handler(message_handler);
    server.set_close_handler(OnClose);
    // 6.设置地址复用
    server.set_reuse_addr(true);   
    // 7.设置服务器监听端口
    server.listen(9000);    
    // 7.启动监听
    server.start_accept();
    // 8.启动服务器
    server.run();
}





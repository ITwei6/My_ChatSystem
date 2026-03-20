#include <elasticlient/client.h>
#include <cpr/cpr.h>
#include <iostream>

int main()
{
    // 构造一个es客户端，用来连接es服务器
    elasticlient::Client client({"http://127.0.0.1:9200/"});
    try
    {
        // 使用es客户端对es服务器进行数据查询
        auto rsp = client.search("user", "_doc", "{\"query\":{\"match_all\" : {}}}");
        // 打印响应状态码和响应正文
        std::cout << rsp.status_code << std::endl;
        std::cout << rsp.text << std::endl;
    }catch(std::exception&e)
    {
        std::cout<<"捕捉到异常，请求失败："<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
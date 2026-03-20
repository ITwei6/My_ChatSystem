#include <gflags/gflags.h>
#include "../../common/es.hpp"
#include<unistd.h>
DEFINE_bool(run_mode, false, "表示程序的运行模式，默认是false调试默认，true表示发布模式");
DEFINE_string(file, "", "表示发布模式下要输出的文件名称，默认调试模式下为空");
DEFINE_int32(level, 0, "表示发布模式下日志器输出的等级，默认调试模式下为0");

int main(int argc, char *argv[])
{
    // 首先初始化gflags框架，告诉它要捕捉命令行中的参数数据
    google::ParseCommandLineFlags(&argc, &argv, true);
    // 初始化spdlog日志器；
    init_logger(FLAGS_run_mode, FLAGS_file, FLAGS_level);

    std::shared_ptr<elasticlient::Client> client(new elasticlient::Client({"http://127.0.0.1:9200/"}));
    bool ret = ESindex(client, "test_user")
                   .append("nickname", "text")
                   .append("phone", "keyword", "standard", true)
                   .create();
    if (ret == false)
    {
        ERROR_LOG("创建索引失败\n");
        return -1;
    }
    else
    {
        INFO_LOG("创建索引成功\n");
    }
    // 插入数据
    ret = ESinsert(client, "test_user")
              .append("nickname", "陶恩威")
              .append("phone", "2268129437")
              .insert("00001");
    if (ret == false)
    {
        ERROR_LOG("插入数据失败\n");
        return -1;
    }
    else
    {
        INFO_LOG("插入数据成功\n");
    }
    // 更新数据
    ret = ESinsert(client, "test_user")
              .append("nickname", "陶恩威")
              .append("phone", "16665205018")
              .insert("00001");
    if (ret == false)
    {
        ERROR_LOG("更新数据失败\n");
        return -1;
    }
    else
    {
        INFO_LOG("更新数据成功\n");
    }
    sleep(2);
    Json::Value rsp =ESsearch(client, "test_user").append_should_match("phone.keyword","16665205018").search();

    if (rsp.empty()||rsp.isArray()==false)
    {
        ERROR_LOG("结果为空或者结果不为数组\n");
        return -1;
    }
    else
    {
        INFO_LOG("检索数据成功\n");
    }
    int sz=rsp.size();
    for(int i=0;i<sz;i++)
    {
        DEBUG_LOG("输出nickname:{}",rsp[i]["_source"]["nickname"].asString());
    }
    ret=ESremove(client, "test_user").remove("00001");
     if (ret == false)
    {
        ERROR_LOG("删除数据失败\n");
        return -1;
    }
    else
    {
        INFO_LOG("删除数据成功\n");
    }
    return 0;
}
#include "../../../common/data_es.hpp"
#include <gflags/gflags.h>

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(es_host, "http://127.0.0.1:9200/", "es服务器URL");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    tew_im::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    auto es_client = tew_im::ESClientFactory::create({FLAGS_es_host});

    auto es_msg = std::make_shared<tew_im::ESMessage>(es_client);
    es_msg->createIndex();

    es_msg->appendData("聊天会话id1", "消息id1", 1769337851, "用户id1", "你叫什么名称");
    es_msg->appendData("聊天会话id1", "消息id2", 1769337851 + 10, "用户id2", "我叫陶恩威,你呢");
    es_msg->appendData("聊天会话id1", "消息id3", 1769337851 + 20, "用户id1", "我叫姚子怡");
    es_msg->appendData("聊天会话id1", "消息id4", 1769337851 + 30, "用户id2", "好的姚子怡");
    auto res = es_msg->search("我", "聊天会话id1");
    for (auto &u : res)
    {
        std::cout << "-----------------" << std::endl;
        std::cout << u.chat_session_id() << std::endl;
        std::cout << u.message_id() << std::endl;
        std::cout << boost::posix_time::to_simple_string(u.create_time()) << std::endl;
        std::cout << u.sender_id() << std::endl;
        std::cout << u.msg_content() << std::endl;
    }
    return 0;
}
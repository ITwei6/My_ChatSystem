
#include "../../../common/mysql_chat_session_member.hpp"
#include "../../../odb/chat_session_member.hxx"
#include "chat_session_member-odb.hxx"
#include <gflags/gflags.h>

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

// void append(tew_im::ChatSessionMemberTable &t)
// {
//     tew_im::ChatSessionMember csm1("聊天会话1", "用户1");
//     t.append(csm1);
//     tew_im::ChatSessionMember csm2("聊天会话1", "用户2");
//     t.append(csm2);
//     tew_im::ChatSessionMember csm3("聊天会话1", "用户3");
//     t.append(csm3);
//     tew_im::ChatSessionMember csm4("聊天会话2", "用户1");
//     t.append(csm4);
//     tew_im::ChatSessionMember csm5("聊天会话2", "用户4");
//     t.append(csm5);
// }
void mutli_append(tew_im::ChatSessionMemberTable &t)
{

    tew_im::ChatSessionMember csm1("聊天会话1", "35ce18e3-10fe-6ebc-0000-0000000000");
    tew_im::ChatSessionMember csm2("聊天会话1", "1d77cbaf-0ca5-cc9d-0000-0000000000");
    std::vector<tew_im::ChatSessionMember> vc({csm1, csm2});
    t.append(vc);
}
void remove(tew_im::ChatSessionMemberTable &t)
{
    t.remove("聊天会话3", "用户6");
}
// std::vector<std::string> get(tew_im::ChatSessionMemberTable &t)
// {
//     std::vector<std::string> vs = t.getuid("聊天会话1");
//     for (auto &uid : vs)
//         std::cout << uid << std::endl;
//     return vs;
// }
void remove_all(tew_im::ChatSessionMemberTable &t)
{
    t.remove_all("聊天会话1");
}
int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    tew_im::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    auto db = tew_im::DataBaseFactory::create("root", "123456", "tew_im", "127.0.0.1", 0, "utf8", 1);

    tew_im::ChatSessionMemberTable csmt(db);
    // append(csmt);
    // mutli_append(csmt);
    // remove(csmt);
    // get(csmt);
    // remove_all(csmt);
    mutli_append(csmt);
    return 0;
}
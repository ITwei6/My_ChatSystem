#include "etcd.hpp"
#include "channel.hpp"
#include "utils.hpp"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <thread>
#include "friend.pb.h"

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(friend_service, "/service/friend_service", "服务监控根目录");

tew_im::ServiceManager::ptr sm;

void apply_test(const std::string &uid1, const std::string &uid2)
{
    auto channel = sm->Choose(FLAGS_friend_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::FriendService_Stub stub(channel.get());
    tew_im::FriendAddReq req;
    tew_im::FriendAddRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid1);
    req.set_respondent_id(uid2);
    brpc::Controller cntl;
    stub.FriendAdd(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

void get_apply_list(const std::string &uid1)
{
    auto channel = sm->Choose(FLAGS_friend_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::FriendService_Stub stub(channel.get());
    tew_im::GetPendingFriendEventListReq req;
    tew_im::GetPendingFriendEventListRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid1);
    brpc::Controller cntl;
    stub.GetPendingFriendEventList(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.event_size(); i++)
    {
        std::cout << "---------------\n";
        std::cout << rsp.event(i).sender().user_id() << std::endl;
        std::cout << rsp.event(i).sender().nickname() << std::endl;
        std::cout << rsp.event(i).sender().avatar() << std::endl;
    }
}

void process_apply_test(const std::string &uid1, bool agree, const std::string &apply_user_id)
{
    auto channel = sm->Choose(FLAGS_friend_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::FriendService_Stub stub(channel.get());
    tew_im::FriendAddProcessReq req;
    tew_im::FriendAddProcessRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid1);
    req.set_agree(agree);
    req.set_apply_user_id(apply_user_id);
    brpc::Controller cntl;
    stub.FriendAddProcess(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    if (agree)
    {
        std::cout << rsp.new_session_id() << std::endl;
    }
}

void search_test(const std::string &uid1, const std::string &key)
{
    auto channel = sm->Choose(FLAGS_friend_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::FriendService_Stub stub(channel.get());
    tew_im::FriendSearchReq req;
    tew_im::FriendSearchRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid1);
    req.set_search_key(key);
    brpc::Controller cntl;
    stub.FriendSearch(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.user_info_size(); i++)
    {
        std::cout << "-------------------\n";
        std::cout << rsp.user_info(i).user_id() << std::endl;
        std::cout << rsp.user_info(i).nickname() << std::endl;
        std::cout << rsp.user_info(i).avatar() << std::endl;
    }
}

void friend_list_test(const std::string &uid1)
{
    auto channel = sm->Choose(FLAGS_friend_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::FriendService_Stub stub(channel.get());
    tew_im::GetFriendListReq req;
    tew_im::GetFriendListRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid1);
    brpc::Controller cntl;
    stub.GetFriendList(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.friend_list_size(); i++)
    {
        std::cout << "-------------------\n";
        std::cout << rsp.friend_list(i).user_id() << std::endl;
        std::cout << rsp.friend_list(i).nickname() << std::endl;
        std::cout << rsp.friend_list(i).avatar() << std::endl;
    }
}

void remove_test(const std::string &uid1, const std::string &uid2)
{
    auto channel = sm->Choose(FLAGS_friend_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::FriendService_Stub stub(channel.get());
    tew_im::FriendRemoveReq req;
    tew_im::FriendRemoveRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid1);
    req.set_peer_id(uid2);
    brpc::Controller cntl;
    stub.FriendRemove(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}
void create_css_test(const std::string &uid1, const std::vector<std::string> &uidlist)
{
    auto channel = sm->Choose(FLAGS_friend_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::FriendService_Stub stub(channel.get());
    tew_im::ChatSessionCreateReq req;
    tew_im::ChatSessionCreateRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid1);
    req.set_chat_session_name("快乐一家人");
    for (auto &id : uidlist)
    {
        req.add_member_id_list(id);
    }
    brpc::Controller cntl;
    stub.ChatSessionCreate(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    std::cout << rsp.chat_session_info().chat_session_id() << std::endl;
    std::cout << rsp.chat_session_info().chat_session_name() << std::endl;
}

void cssmember_test(const std::string &uid1, const std::string &cssid)
{
    auto channel = sm->Choose(FLAGS_friend_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::FriendService_Stub stub(channel.get());
    tew_im::GetChatSessionMemberReq req;
    tew_im::GetChatSessionMemberRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid1);
    req.set_chat_session_id(cssid);
    brpc::Controller cntl;
    stub.GetChatSessionMember(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
    for (int i = 0; i < rsp.member_info_list_size(); i++)
    {
        std::cout << "-------------------\n";
        std::cout << rsp.member_info_list(i).user_id() << std::endl;
        std::cout << rsp.member_info_list(i).nickname() << std::endl;
        std::cout << rsp.member_info_list(i).avatar() << std::endl;
    }
}

void csslist_test(const std::string &uid1)
{
    auto channel = sm->Choose(FLAGS_friend_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::FriendService_Stub stub(channel.get());
    tew_im::GetChatSessionListReq req;
    tew_im::GetChatSessionListRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid1);
    brpc::Controller cntl;
    std::cout << "发送获取聊天会话列表请求！！\n";
    stub.GetChatSessionList(&cntl, &req, &rsp, nullptr);
    std::cout << "请求发送完毕1！！\n";
    ASSERT_FALSE(cntl.Failed());
    std::cout << "请求发送完毕2！！\n";
    ASSERT_TRUE(rsp.success());
    std::cout << "请求发送完毕，且成功！！\n";
    for (int i = 0; i < rsp.chat_session_info_list_size(); i++)
    {
        std::cout << "-------------------\n";
        std::cout << rsp.chat_session_info_list(i).single_chat_friend_id() << std::endl;
        std::cout << rsp.chat_session_info_list(i).chat_session_id() << std::endl;
        std::cout << rsp.chat_session_info_list(i).chat_session_name() << std::endl;
        std::cout << rsp.chat_session_info_list(i).avatar() << std::endl;
        std::cout << "消息内容：\n";
        std::cout << rsp.chat_session_info_list(i).prev_message().message_id() << std::endl;
        std::cout << rsp.chat_session_info_list(i).prev_message().chat_session_id() << std::endl;
        std::cout << rsp.chat_session_info_list(i).prev_message().timestamp() << std::endl;
        std::cout << rsp.chat_session_info_list(i).prev_message().sender().user_id() << std::endl;
        std::cout << rsp.chat_session_info_list(i).prev_message().sender().nickname() << std::endl;
        std::cout << rsp.chat_session_info_list(i).prev_message().sender().avatar() << std::endl;
        std::cout << rsp.chat_session_info_list(i).prev_message().message().file_message().file_name() << std::endl;
        std::cout << rsp.chat_session_info_list(i).prev_message().message().file_message().file_contents() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    tew_im::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    // 1. 先构造Rpc信道管理对象
    sm = std::make_shared<tew_im::ServiceManager>();
    sm->Declared(FLAGS_friend_service);
    auto put_cb = std::bind(&tew_im::ServiceManager::ServiceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&tew_im::ServiceManager::ServiceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    // 2. 构造服务发现对象
    tew_im::Discovery::ptr dclient = std::make_shared<tew_im::Discovery>(FLAGS_etcd_host, put_cb, del_cb);
    dclient->discovery(FLAGS_base_service);
    // apply_test("f5c11f23-6427-8adb-0000-0000000000", "5c917f7b-0c86-6bb2-0000-0000000000");
    // apply_test("92133302-bbf0-2e1a-0000-0000000000", "5c917f7b-0c86-6bb2-0000-0000000000");
    // apply_test("3c50e59e-fb87-63dc-0000-0000000000", "5c917f7b-0c86-6bb2-0000-0000000000");
    // std::cout << "获取陶恩威用户的好友申请列表\n";
    // get_apply_list("5c917f7b-0c86-6bb2-0000-0000000000");
    // std::cout << "-----------------------------------------\n";
    // std::cout << "用户陶恩威处理收到的好友申请，通过姚子怡，小明的申请，拒绝小李的申请\n";
    // process_apply_test("5c917f7b-0c86-6bb2-0000-0000000000", true, "f5c11f23-6427-8adb-0000-0000000000");
    // process_apply_test("5c917f7b-0c86-6bb2-0000-0000000000", false, "3c50e59e-fb87-63dc-0000-0000000000 ");
    // process_apply_test("5c917f7b-0c86-6bb2-0000-0000000000", true, "92133302-bbf0-2e1a-0000-0000000000");
    // std::cout << "陶恩威用户搜索：\n";
    // search_test("5c917f7b-0c86-6bb2-0000-0000000000", "小");
    // std::cout << "姚子怡用户搜索：\n";
    // search_test("f5c11f23-6427-8adb-0000-0000000000", "小");
    // std::cout << "小李用户搜索：\n";
    // search_test("3c50e59e-fb87-63dc-0000-0000000000", "小");
    std::cout << "获取陶恩威用户的好友列表\n";
    friend_list_test("5c917f7b-0c86-6bb2-0000-0000000000");
    std::cout << "获取姚子怡用户的好友列表\n";
    friend_list_test("f5c11f23-6427-8adb-0000-0000000000");
    std::cout << "获取小李用户的好友列表\n";
    friend_list_test("3c50e59e-fb87-63dc-0000-0000000000");
    // remove_test("c4dc-68239a9a-0001", "053f-04e5e4c5-0001");
    // std::vector<std::string> uidlist = {
    //     "731f-50086884-0000",
    //     "c4dc-68239a9a-0001",
    //     "31ab-86a1209d-0000",
    //     "053f-04e5e4c5-0001"};
    // create_css_test("731f-50086884-0000", uidlist);
    // cssmember_test("731f-50086884-0000", "36b5-edaf4987-0000");
    // std::cout << "++++++++++++++++++++++\n";
    // cssmember_test("c4dc-68239a9a-0001", "36b5-edaf4987-0000");

    // csslist_test("c4dc-68239a9a-0001");
    return 0;
}
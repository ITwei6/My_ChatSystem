// #include "etcd.hpp"
// #include <gflags/gflags.h>
// #include <thread>
// #include <brpc/channel.h>
// #include "transmit.pb.h"
// #include "user.pb.h"
// #include "base.pb.h"
// #include "channel.hpp"

// // 想通过命令行参数调整变量的内容，所以通过gflags框架捕捉命令行参数数据放入到自己定义的全局变量中
// DEFINE_bool(run_mode, false, "表示程序的运行模式，默认是false调试默认，true表示发布模式");
// DEFINE_string(file, "", "表示发布模式下要输出的文件名称，默认调试模式下为空");
// DEFINE_int32(level, 0, "表示发布模式下日志器输出的等级，默认调试模式下为0");

// DEFINE_string(host, "http://127.0.0.1:2379", "服务注册中心的地址");
// DEFINE_string(basedir, "/service", "即服务要监控的目录");
// DEFINE_string(call_service, "/service/transmit_service", "要查找的服务名称");

// // 1.首先构造一个Rpc所有服务管理对象 2.构造一个服务发现对象 3.通过Rpc信道管理对象，获取提供echo服务的信道 4.发起echorpc调用
// int main(int argc, char *argv[])
// {
//     // 首先初始化gflags框架，告诉它要捕捉命令行中的参数数据
//     google::ParseCommandLineFlags(&argc, &argv, true);
//     // 初始化spdlog日志器；
//     tew_im::init_logger(FLAGS_run_mode, FLAGS_file, FLAGS_level);

//     // 1.首先构造一个Rpc所有服务管理对象
//     auto service_manager = std::make_shared<tew_im::ServiceManager>();
//     service_manager->Declared(FLAGS_call_service); // 提前声明关心什么服务
//     auto put_cb = std::bind(&tew_im::ServiceManager::ServiceOnline, service_manager.get(), std::placeholders::_1, std::placeholders::_2);
//     auto delete_cb = std::bind(&tew_im::ServiceManager::ServiceOffline, service_manager.get(), std::placeholders::_1, std::placeholders::_2);
//     // 2.构造一个服务发现对象
//     tew_im::Discovery::ptr dclient = std::make_shared<tew_im::Discovery>(FLAGS_host, put_cb, delete_cb);
//     // 注册的服务是：/service/speech/instance ：127.0.0.1:8888
//     dclient->discovery(FLAGS_basedir);

//     // 4. 发起Rpc调用
//     auto channel = service_manager->Choose(FLAGS_call_service);
//     if (!channel)
//     {
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//         return -1;
//     }

//     // 构建stub类对象，用于进行rpc调用
//     tew_im::MsgTransmitService_Stub stub(channel.get());
//     // 进行rpc调用
//     brpc::Controller *controler = new brpc::Controller();
//     tew_im::NewMessageReq req;
//     req.set_request_id("2222");
//     req.set_user_id("35ce18e3-10fe-6ebc-0000-0000000000");
//     req.set_chat_session_id("聊天会话1");
//     req.mutable_message()->set_message_type(tew_im::MessageType::STRING);
//     req.mutable_message()->mutable_string_message()->set_content("hello你们好呀，这是一条测试消息");

//     tew_im::GetTransmitTargetRsp *rsp = new tew_im::GetTransmitTargetRsp();
//     // 同步模式下使用这个
//     stub.GetTransmitTarget(controler, &req, rsp, nullptr);
//     if (controler->Failed() == true || rsp->success() == false)
//     {
//         std::cout << "rpc调用失败" << controler->ErrorText() << std::endl;
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//         delete controler;
//         delete rsp;
//         return -1;
//     }
//     std::cout << rsp->request_id() << std::endl;
//     std::cout << "-------组织完毕的消息：---------" << std::endl;
//     std::cout << rsp->message().chat_session_id() << std::endl;
//     std::cout << rsp->message().timestamp() << std::endl;
//     std::cout << "发送者详细信息：";
//     std::cout << rsp->message().sender().user_id() << std::endl;
//     std::cout << rsp->message().sender().nickname() << std::endl;
//     std::cout << "-------转发客户端列表：---------" << std::endl;
//     for (int i = 0; i < rsp->target_id_list_size(); i++)
//     {
//         std::cout << rsp->target_id_list(i) << std::endl;
//     }
//     delete controler;
//     delete rsp;
//     return 0;
// }

#include "etcd.hpp"
#include "channel.hpp"
#include "utils.hpp"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <thread>
#include "transmit.pb.h"

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(transmite_service, "/service/transmit_service", "服务监控根目录");

tew_im::ServiceManager::ptr sm;

void string_message(const std::string &uid, const std::string &sid, const std::string &msg)
{
    auto channel = sm->Choose(FLAGS_transmite_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::MsgTransmitService_Stub stub(channel.get());
    tew_im::NewMessageReq req;
    tew_im::GetTransmitTargetRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid);
    req.set_chat_session_id(sid);
    req.mutable_message()->set_message_type(tew_im::MessageType::STRING);
    req.mutable_message()->mutable_string_message()->set_content(msg);
    brpc::Controller cntl;
    stub.GetTransmitTarget(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}
void image_message(const std::string &uid, const std::string &sid, const std::string &msg)
{
    auto channel = sm->Choose(FLAGS_transmite_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::MsgTransmitService_Stub stub(channel.get());
    tew_im::NewMessageReq req;
    tew_im::GetTransmitTargetRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid);
    req.set_chat_session_id(sid);
    req.mutable_message()->set_message_type(tew_im::MessageType::IMAGE);
    req.mutable_message()->mutable_image_message()->set_image_content(msg);
    brpc::Controller cntl;
    stub.GetTransmitTarget(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

void speech_message(const std::string &uid, const std::string &sid, const std::string &msg)
{
    auto channel = sm->Choose(FLAGS_transmite_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::MsgTransmitService_Stub stub(channel.get());
    tew_im::NewMessageReq req;
    tew_im::GetTransmitTargetRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid);
    req.set_chat_session_id(sid);
    req.mutable_message()->set_message_type(tew_im::MessageType::SPEECH);
    req.mutable_message()->mutable_speech_message()->set_file_contents(msg);
    brpc::Controller cntl;
    stub.GetTransmitTarget(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

void file_message(const std::string &uid, const std::string &sid,
                  const std::string &filename, const std::string &content)
{
    auto channel = sm->Choose(FLAGS_transmite_service);
    if (!channel)
    {
        std::cout << "获取通信信道失败！" << std::endl;
        return;
    }
    tew_im::MsgTransmitService_Stub stub(channel.get());
    tew_im::NewMessageReq req;
    tew_im::GetTransmitTargetRsp rsp;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid);
    req.set_chat_session_id(sid);
    req.mutable_message()->set_message_type(tew_im::MessageType::FILE);
    req.mutable_message()->mutable_file_message()->set_file_contents(content);
    req.mutable_message()->mutable_file_message()->set_file_name(filename);
    req.mutable_message()->mutable_file_message()->set_file_size(content.size());
    brpc::Controller cntl;
    stub.GetTransmitTarget(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    tew_im::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    // 1. 先构造Rpc信道管理对象
    sm = std::make_shared<tew_im::ServiceManager>();
    sm->Declared(FLAGS_transmite_service);
    auto put_cb = std::bind(&tew_im::ServiceManager::ServiceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&tew_im::ServiceManager::ServiceOffline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    // 2. 构造服务发现对象
    tew_im::Discovery::ptr dclient = std::make_shared<tew_im::Discovery>(FLAGS_etcd_host, put_cb, del_cb);
    dclient->discovery(FLAGS_base_service);
    // 3. 通过Rpc信道管理对象，获取提供Echo服务的信道
    string_message("35ce18e3-10fe-6ebc-0000-0000000000", "聊天会话1", "吃饭了吗？");
    string_message("1d77cbaf-0ca5-cc9d-0000-0000000000", "聊天会话1", "吃的盖浇饭！！");
    image_message("35ce18e3-10fe-6ebc-0000-0000000000", "聊天会话1", "可爱表情图片数据");
    speech_message("35ce18e3-10fe-6ebc-0000-0000000000", "聊天会话1", "动听猪叫声数据");
    file_message("35ce18e3-10fe-6ebc-0000-0000000000", "聊天会话1", "猪爸爸的文件名称", "猪爸爸的文件数据");
    return 0;
}
#include "etcd.hpp"
#include "channel.hpp"
#include "utils.hpp"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <thread>
#include "user.pb.h"
#include "base.pb.h"

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(user_service, "/service/user_service", "服务监控根目录");

tew_im::ServiceManager::ptr user_channels;
void reg_user(const std::string &nickname, const std::string &pswd)
{
    auto channel = user_channels->Choose(FLAGS_user_service); // 获取通信信道
    ASSERT_TRUE(channel);

    tew_im::UserRegisterReq req;
    req.set_request_id(tew_im::Uuid());
    req.set_nickname(nickname);
    req.set_password(pswd);

    tew_im::UserRegisterRsp rsp;
    brpc::Controller cntl;
    tew_im::UserService_Stub stub(channel.get());
    stub.UserRegister(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

void set_user_avatar(const std::string &uid, const std::string &avatar)
{
    auto channel = user_channels->Choose(FLAGS_user_service); // 获取通信信道
    ASSERT_TRUE(channel);
    tew_im::SetUserAvatarReq req;
    req.set_request_id(tew_im::Uuid());
    req.set_user_id(uid);
    req.set_session_id("测试登录会话ID");
    req.set_avatar(avatar);
    tew_im::SetUserAvatarRsp rsp;
    brpc::Controller cntl;
    tew_im::UserService_Stub stub(channel.get());
    stub.SetUserAvatar(&cntl, &req, &rsp, nullptr);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_TRUE(rsp.success());
}

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    tew_im::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    user_channels = std::make_shared<tew_im::ServiceManager>();

    user_channels->Declared(FLAGS_user_service);
    auto put_cb = std::bind(&tew_im::ServiceManager::ServiceOnline, user_channels.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&tew_im::ServiceManager::ServiceOnline, user_channels.get(), std::placeholders::_1, std::placeholders::_2);

    // 2. 构造服务发现对象
    tew_im::Discovery::ptr dclient = std::make_shared<tew_im::Discovery>(FLAGS_etcd_host, put_cb, del_cb);
    dclient->discovery(FLAGS_base_service);
    reg_user("小明", "123456");
    reg_user("小李", "123456");
    std::string uid1, uid2;
    std::cout << "输入小明用户ID：";
    std::fflush(stdout);
    std::cin >> uid1;
    std::cout << "输入小李用户ID：";
    std::fflush(stdout);
    std::cin >> uid2;
    set_user_avatar(uid1, "小明头像数据");
    set_user_avatar(uid2, "小李头像数据");
    return 0;
}
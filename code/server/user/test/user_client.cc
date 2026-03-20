
// #include <gflags/gflags.h>
// #include <gtest/gtest.h>
// #include <thread>
// #include "etcd.hpp"
// #include "channel.hpp"
// #include "logger.hpp"
// #include "file.pb.h"
// #include "base.pb.h"
// #include "utils.hpp"
// #include "user.pb.h"

// DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
// DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
// DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

// DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址");
// DEFINE_string(base_service, "/service", "服务监控根目录");
// DEFINE_string(user_service, "/service/user_service", "服务监控根目录");

// tew_im::ServiceChannel::ChannelPtr channel;
// tew_im::UserInfo userinfo;
// std::string session_id; // 用户登录之后会返回一个用户会话id，非常重要需要保存起来，用户登录后续的操作都是基于会话id，网关找到用户id来操作
// std::string new_name = "小胖";
// // TEST(用户子服务测试, 用户注册测试)
// // {
// //     // 2. 实例化rpc调用客户端对象，发起rpc调用
// //     tew_im::UserService_Stub stub(channel.get());

// //     tew_im::UserRegisterReq req;
// //     brpc::Controller cntl;
// //     tew_im::UserRegisterRsp rsp;
// //     req.set_request_id(tew_im::Uuid());
// //     req.set_nickname(userinfo.nickname());
// //     req.set_password("123456");
// //     stub.UserRegister(&cntl, &req, &rsp, nullptr);
// //     ASSERT_FALSE(cntl.Failed());
// //     // 3. 检测返回值中上传是否成功
// //     ASSERT_TRUE(rsp.success());
// // }

// TEST(用户子服务测试, 用户登录测试)
// {
//     // 2. 实例化rpc调用客户端对象，发起rpc调用
//     tew_im::UserService_Stub stub(channel.get());
//     tew_im::UserLoginReq req;
//     brpc::Controller cntl;
//     tew_im::UserLoginRsp rsp;
//     req.set_request_id(tew_im::Uuid());
//     std::cout << req.request_id() << std::endl;
//     req.set_nickname("大耳朵图图");
//     std::cout << req.nickname() << std::endl;
//     req.set_password("123456");
//     std::cout << req.password() << std::endl;
//     stub.UserLogin(&cntl, &req, &rsp, nullptr);
//     ASSERT_FALSE(cntl.Failed());
//     // 3. 检测返回值中上传是否成功
//     ASSERT_TRUE(rsp.success());
//     session_id = rsp.login_session_id();
// }
// // TEST(用户子服务测试, 用户头像设置测试)
// // {
// //     // 2. 实例化rpc调用客户端对象，发起rpc调用
// //     tew_im::UserService_Stub stub(channel.get());
// //     tew_im::SetUserAvatarReq req;
// //     brpc::Controller cntl;
// //     tew_im::SetUserAvatarRsp rsp;
// //     req.set_request_id(tew_im::Uuid());
// //     req.set_session_id(session_id);
// //     req.set_user_id(userinfo.user_id());
// //     req.set_avatar(userinfo.avatar());
// //     stub.SetUserAvatar(&cntl, &req, &rsp, nullptr);
// //     ASSERT_FALSE(cntl.Failed());
// //     // 3. 检测返回值中上传是否成功
// //     ASSERT_TRUE(rsp.success());
// // }
// // TEST(用户子服务测试, 用户签命设置测试)
// // {
// //     // 2. 实例化rpc调用客户端对象，发起rpc调用
// //     tew_im::UserService_Stub stub(channel.get());

// //     tew_im::SetUserDescriptionReq req;
// //     brpc::Controller cntl;
// //     tew_im::SetUserDescriptionRsp rsp;
// //     req.set_request_id(tew_im::Uuid());
// //     req.set_session_id(session_id);
// //     req.set_user_id(userinfo.user_id());
// //     req.set_description(userinfo.description());
// //     stub.SetUserDescription(&cntl, &req, &rsp, nullptr);
// //     ASSERT_FALSE(cntl.Failed());
// //     // 3. 检测返回值中上传是否成功
// //     ASSERT_TRUE(rsp.success());
// // }

// // TEST(用户子服务测试, 用户昵称设置测试) {
// //      //2. 实例化rpc调用客户端对象，发起rpc调用
// //     tew_im::UserService_Stub stub(channel.get());
// //     tew_im::SetUserNicknameReq req;
// //     brpc::Controller cntl;
// //     tew_im::SetUserNicknameRsp rsp;
// //     req.set_request_id(tew_im::Uuid());
// //     req.set_session_id(session_id);
// //     req.set_user_id(userinfo.user_id());
// //     req.set_nickname(new_name);
// //     stub.SetUserNickname(&cntl, &req, &rsp, nullptr);
// //     ASSERT_FALSE(cntl.Failed());
// //     //3. 检测返回值中上传是否成功
// //     ASSERT_TRUE(rsp.success());
// // }
// // TEST(用户子服务测试, 用户信息获取测试) {
// //      //2. 实例化rpc调用客户端对象，发起rpc调用
// //     tew_im::UserService_Stub stub(channel.get());
// //     tew_im::GetUserInfoReq req;
// //     brpc::Controller cntl;
// //     tew_im::GetUserInfoRsp rsp;
// //     req.set_request_id(tew_im::Uuid());
// //     req.set_session_id(session_id);
// //     req.set_user_id(userinfo.user_id());
// //     stub.GetUserInfo(&cntl, &req, &rsp, nullptr);
// //     ASSERT_FALSE(cntl.Failed());
// //     //3. 检测返回值中上传是否成功
// //     ASSERT_TRUE(rsp.success());
// //     ASSERT_EQ(userinfo.user_id(),rsp.user_info().user_id());
// //     ASSERT_EQ(new_name,rsp.user_info().nickname());
// //     ASSERT_EQ(userinfo.description(),rsp.user_info().description());
// //     ASSERT_EQ(userinfo.avatar(),rsp.user_info().avatar());
// // }

// // TEST(用户子服务测试, 批量用户信息获取测试)
// // {
// //     // 2. 实例化rpc调用客户端对象，发起rpc调用
// //     tew_im::UserService_Stub stub(channel.get());
// //     tew_im::GetMultiUserInfoReq req;
// //     brpc::Controller cntl;
// //     tew_im::GetMultiUserInfoRsp rsp;
// //     req.set_request_id(tew_im::Uuid());
// //     req.add_users_id("e87f675b-c4f5-40e7-0000-0000000000");
// //     req.add_users_id("fde58570-ddc8-a0d4-0000-0000000000");
// //     stub.GetMultiUserInfo(&cntl, &req, &rsp, nullptr);
// //     ASSERT_FALSE(cntl.Failed());
// //     // 3. 检测返回值中上传是否成功
// //     ASSERT_TRUE(rsp.success());

// //     //    map<string, UserInfo> users_info = 4;
// //     auto user_map = rsp.mutable_users_info();
// //     auto t_user = (*user_map)["e87f675b-c4f5-40e7-0000-0000000000"];
// //     auto y_user = (*user_map)["fde58570-ddc8-a0d4-0000-0000000000"];

// //     ASSERT_EQ(t_user.user_id(), "e87f675b-c4f5-40e7-0000-0000000000");
// //     ASSERT_EQ(t_user.nickname(), "小胖");
// //     ASSERT_EQ(t_user.phone(), "");
// //     ASSERT_EQ(t_user.description(), "小陶真厉害");
// //     ASSERT_EQ(t_user.avatar(), "小陶头像");

// //     ASSERT_EQ(y_user.user_id(), "fde58570-ddc8-a0d4-0000-0000000000");
// //     ASSERT_EQ(y_user.nickname(), "小姚");
// //     ASSERT_EQ(y_user.phone(), "");
// //     ASSERT_EQ(y_user.description(), "小姚真厉害");
// //     ASSERT_EQ(y_user.avatar(), "小姚头像");
// // }

// int main(int argc, char *argv[])
// {
//     testing::InitGoogleTest(&argc, argv);
//     google::ParseCommandLineFlags(&argc, &argv, true);

//     tew_im::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

//     // 1. 先构造Rpc信道管理对象
//     auto sm = std::make_shared<tew_im::ServiceManager>();
//     sm->Declared(FLAGS_user_service);
//     auto put_cb = std::bind(&tew_im::ServiceManager::ServiceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
//     auto del_cb = std::bind(&tew_im::ServiceManager::ServiceOffline, sm.get(), std::placeholders::_1, std::placeholders::_2);
//     // 2. 构造服务发现对象
//     tew_im::Discovery::ptr dclient = std::make_shared<tew_im::Discovery>(FLAGS_etcd_host, put_cb, del_cb);
//     dclient->discovery(FLAGS_base_service);

//     // 3. 通过Rpc信道管理对象，获取提供服务的信道
//     channel = sm->Choose(FLAGS_user_service);
//     if (!channel)
//     {
//         std::this_thread::sleep_for(std::chrono::seconds(1));
//         return -1;
//     }
//     // 构建一个用户对象信息
//     userinfo.set_nickname("大耳朵图图");
//     userinfo.set_description("智商250");
//     userinfo.set_phone("1555566688");
//     userinfo.set_avatar("图图头像");
//     userinfo.set_user_id("1d77cbaf-0ca5-cc9d-0000-0000000000"); // 这个应该是由网关通过会话健全来设置的
//     return RUN_ALL_TESTS();
// }
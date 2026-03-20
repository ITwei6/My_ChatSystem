// 实现语音识别子服务
#include <brpc/server.h>
#include <butil/logging.h>

#include "data_es.hpp"    // es数据管理客户端封装
#include "data_redis.hpp" // redis数据管理客户端封装
#include "data_mysql.hpp" // mysql数据管理客户端封装
#include "etcd.hpp"       // 服务注册模块封装
#include "logger.hpp"     // 日志模块封装
#include "utils.hpp"      // 基础工具接口

#include "channel.hpp" // 信道管理模块封装

#include "user.hxx"
#include "user-odb.hxx"

#include "user.pb.h" // protobuf框架代码
#include "base.pb.h" // protobuf框架代码
#include "file.pb.h" // protobuf框架代码

namespace tew_im
{
    class UserServiceImpl : public tew_im::UserService
    {
    public:
        UserServiceImpl(
            const std::shared_ptr<elasticlient::Client> &es_client,
            const std::shared_ptr<odb::core::database> &mysql_client,
            const std::shared_ptr<sw::redis::Redis> &redis_client,
            const ServiceManager::ptr &channel_manager,
            const std::string &file_service_name) : _es_user(std::make_shared<ESUser>(es_client)),
                                                    _mysql_user(std::make_shared<UserTable>(std::dynamic_pointer_cast<odb::mysql::database>(mysql_client))),

                                                    _redis_session(std::make_shared<Session>(redis_client)),
                                                    _redis_status(std::make_shared<Status>(redis_client)),
                                                    _redis_codes(std::make_shared<Codes>(redis_client)),
                                                    _file_service_name(file_service_name),
                                                    _mm_channels(channel_manager)
        {
            _es_user->createIndex();
        }
        ~UserServiceImpl() {}
        bool check_name(const std::string &nickname)
        {
            return nickname.size() < 25;
        }
        bool check_passwd(const std::string &passwd)
        {
            if (passwd.size() < 6 || passwd.size() > 15)
            {
                ERROR_LOG("密码长度不合法");
                return false;
            }
            for (auto &ch : passwd)
            {
                if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')))
                {
                    ERROR_LOG("密码字符不合法");
                    return false;
                }
            }
            return true;
        }
        virtual void UserRegister(::google::protobuf::RpcController *controller,
                                  const ::tew_im::UserRegisterReq *request,
                                  ::tew_im::UserRegisterRsp *response,
                                  ::google::protobuf::Closure *done)
        {
            DEBUG_LOG("收到用户注册请求！");
            brpc::ClosureGuard rpc_guard(done);
            // 先构建一个错误响应回调函数
            auto err_response = [this, response](const std::string &uid, const std::string &err_msg)
            {
                response->set_request_id(uid);
                response->set_success(false);
                response->set_errmsg(err_msg);
            };
            // 1. 从请求中取出昵称和密码
            std::string nickname = request->nickname();
            std::string password = request->password();
            // 2. 检查昵称是否合法（长度限制 3~15 之间）
            if (!check_name(nickname))
            {
                ERROR_LOG("{}：用户昵称不合法", request->request_id());
                err_response(request->request_id(), "昵称不合法");
                return;
            }
            // 3. 检查密码是否合法（只能包含字母，数字，长度限制 6~15 之间）
            if (!check_passwd(password))
            {
                ERROR_LOG("{}：用户密码不合法", request->request_id());
                err_response(request->request_id(), "密码不合法");
                return;
            }
            // 4. 根据昵称在数据库进行判断是否昵称已存在
            auto user = _mysql_user->select_by_nickname(nickname);
            if (user)
            {
                ERROR_LOG("{}：用户信息已经存在", request->request_id());
                err_response(request->request_id(), "用户已经存在");
                return;
            }
            // 5. 向数据库新增数据
            std::string uid = Uuid();
            bool ret = _mysql_user->insert(std::make_shared<User>(uid, nickname, password));
            if (!ret)
            {
                ERROR_LOG("{}：向mysql中插入新用户失败", request->request_id());
                err_response(request->request_id(), "mysql中添加用户信息失败");
                return;
            }
            // 6. 向 ES 服务器中新增用户信息

            ret = _es_user->appendData(uid, "", nickname, "", "");
            if (!ret)
            {
                ERROR_LOG("{}：向es搜索引擎中插入用户信息失败", request->request_id());
                err_response(request->request_id(), "es中添加用户信息失败");
                return;
            }
            // 7. 组织响应，进行成功与否的响应即可。
            response->set_request_id(request->request_id());
            response->set_success(true);
        }
        virtual void UserLogin(::google::protobuf::RpcController *controller,
                               const ::tew_im::UserLoginReq *request,
                               ::tew_im::UserLoginRsp *response,
                               ::google::protobuf::Closure *done)
        {
            DEBUG_LOG("收到用户登录请求！");
            brpc::ClosureGuard rpc_guard(done);
            // 先构建一个错误响应回调函数
            auto err_response = [this, response](const std::string &uid, const std::string &err_msg)
            {
                response->set_request_id(uid);
                response->set_success(false);
                response->set_errmsg(err_msg);
            };
            // 1. 从请求中取出昵称和密码
            std::cout << "从请求中取出request_id:" << request->request_id() << std::endl;
            std::string nickname = request->nickname();
            std::cout << "从请求中取出昵称：" << nickname << std::endl;
            std::string password = request->password();
            std::cout << "从请求中取出密码：" << password << std::endl;
            // 2. 通过昵称获取用户信息，进行密码是否一致的判断
            auto user = _mysql_user->select_by_nickname(nickname);
            if (!user || password != user->password())
            {
                ERROR_LOG("{}：mysql中用户不存在或者密码不正确:{}", request->request_id(), password);
                err_response(request->request_id(), "用户不存在或密码不一致");
                return;
            }
            // 3. 根据 redis 中的登录标记信息是否存在判断用户是否已经登录。
            bool ret = _redis_status->isexists(user->user_id());
            if (ret)
            {
                ERROR_LOG("{}：用户已经在别处登录{}", nickname, request->request_id());
                err_response(request->request_id(), "用户重复登录");
                return;
            }
            // 4. 构造会话 ID，生成会话键值对，向 redis 中添加会话信息以及登录标记信息
            std::string sid = Uuid();
            _redis_session->insert(sid, user->user_id());
            _redis_status->insert(user->user_id());
            // 5. 组织响应，返回生成的会话 ID
            response->set_login_session_id(sid);
            response->set_request_id(request->request_id());
            response->set_success(true);
        }
        virtual void GetPhoneVerifyCode(::google::protobuf::RpcController *controller,
                                        const ::tew_im::PhoneVerifyCodeReq *request,
                                        ::tew_im::PhoneVerifyCodeRsp *response,
                                        ::google::protobuf::Closure *done)
        {
            DEBUG_LOG("收到短信验证码获取请求！");
            brpc::ClosureGuard rpc_guard(done);
        }
        virtual void PhoneRegister(::google::protobuf::RpcController *controller,
                                   const ::tew_im::PhoneRegisterReq *request,
                                   ::tew_im::PhoneRegisterRsp *response,
                                   ::google::protobuf::Closure *done)
        {
            DEBUG_LOG("收到手机号注册请求！");
            brpc::ClosureGuard rpc_guard(done);
        }

        virtual void GetUserInfo(::google::protobuf::RpcController *controller,
                                 const ::tew_im::GetUserInfoReq *request,
                                 ::tew_im::GetUserInfoRsp *response,
                                 ::google::protobuf::Closure *done)
        {
            DEBUG_LOG("收到单个用户信息获取请求！");
            brpc::ClosureGuard rpc_guard(done);
            // 先构建一个错误响应回调函数
            auto err_response = [this, response](const std::string &uid, const std::string &err_msg)
            {
                response->set_request_id(uid);
                response->set_success(false);
                response->set_errmsg(err_msg);
            };
            // 1. 从请求中取出用户 ID(网关设置的)
            std::string uid = request->user_id();
            // 2. 通过用户 ID，从数据库中查询用户信息
            auto user = _mysql_user->select_by_user_id(uid);
            if (!user)
            {
                ERROR_LOG("{}：用户信息不存在", request->request_id());
                err_response(request->request_id(), "用户信息不存在");
                return;
            }
            // 用户的头像id一开始可能不存在的，如果获取到的用户信息中文件id未空，则就不用调用文件子服务了
            // 3. 根据用户信息中的头像 ID，从文件服务器获取头像文件数据，组织完整用户信息
            auto user_info = response->mutable_user_info();
            user_info->set_user_id(user->user_id());
            user_info->set_nickname(user->nickname());
            user_info->set_description(user->description());
            user_info->set_phone(user->phone());

            if (!user->avatar_id().empty())
            {
                // 根据信道管理对象，来获取到连接到文件子服务的信道
                auto channel = _mm_channels->Choose(_file_service_name);
                if (!channel)
                {
                    ERROR_LOG("{}：文件子服务信道对象不存在:{}", request->request_id());
                    err_response(request->request_id(), "文件子服务信道不存在");
                    return;
                }
                tew_im::FileService_Stub stub(channel.get());
                tew_im::GetSingleFileReq req;
                tew_im::GetSingleFileRsp rsp;
                req.set_request_id(request->request_id());
                req.set_file_id(user->avatar_id());
                brpc::Controller cntl;
                stub.GetSingleFile(&cntl, &req, &rsp, nullptr);
                if (cntl.Failed() == true || rsp.success() == false)
                {
                    ERROR_LOG("{}：调用文件子服务失败:{}", request->request_id(), cntl.ErrorText());
                    err_response(request->request_id(), "获取头像数据失败");
                    return;
                }
                // 调用成功，头像文件数据就在响应中
                user_info->set_avatar(rsp.file_data().file_content());
            }
            // 4. 组织响应，返回用户信息
            response->set_request_id(request->request_id());
            response->set_success(true);
        }

        virtual void GetMultiUserInfo(::google::protobuf::RpcController *controller,
                                      const ::tew_im::GetMultiUserInfoReq *request,
                                      ::tew_im::GetMultiUserInfoRsp *response,
                                      ::google::protobuf::Closure *done)
        {
            DEBUG_LOG("收到批量用户信息获取请求！");
            brpc::ClosureGuard rpc_guard(done);
            // 先构建一个错误响应回调函数
            auto err_response = [this, response](const std::string &uid, const std::string &err_msg)
            {
                response->set_request_id(uid);
                response->set_success(false);
                response->set_errmsg(err_msg);
            };
            // 1.首先从请求中获取到用户id列表
            std::vector<std::string> id_list;
            for (int i = 0; i < request->users_id_size(); i++)
            {
                id_list.push_back(request->users_id(i));
            }
            // 2.通过用户id列表获取多个用户信息(主要是头像id)
            std::vector<User> users = _mysql_user->select_by_mutli_id(id_list);
            if (users.size() != request->users_id_size())
            {
                ERROR_LOG("{}：从数据库中获取的用户信息数量不一致:{}-{}", request->request_id(), users.size(), request->users_id_size());
                err_response(request->request_id(), "获取用户信息数量不一致");
                return;
            }
            // 3.根据用户信息中的头像 ID，从文件服务器获取头像文件数据，组织完整用户信息
            // 根据信道管理对象，来获取到连接到文件子服务的信道
            auto channel = _mm_channels->Choose(_file_service_name);
            if (!channel)
            {
                ERROR_LOG("{}：文件子服务信道对象不存在:{}", request->request_id());
                err_response(request->request_id(), "文件子服务信道不存在");
                return;
            }
            tew_im::FileService_Stub stub(channel.get());
            tew_im::GetMultiFileReq req;
            tew_im::GetMultiFileRsp rsp;
            brpc::Controller cntl;
            req.set_request_id(request->request_id());
            for (auto &user : users)
            {
                if (user.avatar_id().empty())
                    continue;
                req.add_file_id_list(user.avatar_id());
            }
            stub.GetMultiFile(&cntl, &req, &rsp, nullptr);
            if (cntl.Failed() == true || rsp.success() == false)
            {
                ERROR_LOG("{}：调用多文件子服务失败:{}", request->request_id(), cntl.ErrorText());
                err_response(request->request_id(), "获取头像数据失败");
                return;
            }
            auto file_map = rsp.mutable_file_data(); // 这是文件子服务响应中获取所有头像id对应的数据map
            //<string, FileDownloadData>
            // 填充本次请求的响应结果
            response->set_request_id(request->request_id());
            auto user_map = response->mutable_users_info(); // 这是填充用户id与对应用户数据map
            //<string, UserInfo>
            for (auto &user : users)
            {
                UserInfo uf;
                uf.set_user_id(user.user_id());
                uf.set_nickname(user.nickname());
                uf.set_description(user.description());
                uf.set_phone(user.phone());
                if (!user.avatar_id().empty())
                    uf.set_avatar((*file_map)[user.avatar_id()].file_content());
                else
                    uf.set_avatar("");
                // 无头像ID，直接设为空
                (*user_map)[user.user_id()] = uf;
            }
            response->set_success(true);
        }
        virtual void SetUserAvatar(::google::protobuf::RpcController *controller,
                                   const ::tew_im::SetUserAvatarReq *request,
                                   ::tew_im::SetUserAvatarRsp *response,
                                   ::google::protobuf::Closure *done)
        {
            DEBUG_LOG("收到用户头像设置请求！");
            brpc::ClosureGuard rpc_guard(done);
            // 先构建一个错误响应回调函数
            auto err_response = [this, response](const std::string &uid, const std::string &err_msg)
            {
                response->set_request_id(uid);
                response->set_success(false);
                response->set_errmsg(err_msg);
            };
            // 1. 从请求中取出用户 ID 与头像数据
            std::string uid = request->user_id();
            auto avater_data = request->avatar();
            // 2. 从数据库通过用户 ID 进行用户信息查询，判断用户是否存在
            auto user = _mysql_user->select_by_user_id(uid);
            if (!user)
            {
                ERROR_LOG("{}：用户信息不存在", request->request_id());
                err_response(request->request_id(), "用户信息不存在");
                return;
            }
            // 3. 上传头像文件到文件子服务，
            // 根据信道管理对象，来获取到连接到文件子服务的信道
            auto channel = _mm_channels->Choose(_file_service_name);
            if (!channel)
            {
                ERROR_LOG("{}：文件子服务信道对象不存在:{}", request->request_id());
                err_response(request->request_id(), "文件子服务信道不存在");
                return;
            }
            tew_im::FileService_Stub stub(channel.get());
            tew_im::PutSingleFileReq req;
            tew_im::PutSingleFileRsp rsp;
            brpc::Controller cntl;
            req.set_request_id(request->request_id());
            req.mutable_file_data()->set_file_size(avater_data.size());
            req.mutable_file_data()->set_file_content(avater_data);

            stub.PutSingleFile(&cntl, &req, &rsp, nullptr);
            if (cntl.Failed() == true || rsp.success() == false)
            {
                ERROR_LOG("{}：调用文件子服务失败:{}", request->request_id(), cntl.ErrorText());
                err_response(request->request_id(), "上传头像数据失败");
                return;
            }
            // 4. 将返回的头像文件 ID 更新到数据库中
            auto avater_id = rsp.file_info().file_id();
            user->avatar_id(avater_id);
            _mysql_user->update(user);
            // 5. 更新 ES 服务器中用户信息
            _es_user->appendData(user->user_id(), user->phone(), user->nickname(), user->description(), avater_id);
            // 6. 组织响应，返回更新成功与否
            response->set_request_id(request->request_id());
            response->set_success(true);
        }
        virtual void SetUserNickname(::google::protobuf::RpcController *controller,
                                     const ::tew_im::SetUserNicknameReq *request,
                                     ::tew_im::SetUserNicknameRsp *response,
                                     ::google::protobuf::Closure *done)
        {
            DEBUG_LOG("收到用户昵称设置请求！");
            brpc::ClosureGuard rpc_guard(done);
            // 先构建一个错误响应回调函数
            auto err_response = [this, response](const std::string &uid, const std::string &err_msg)
            {
                response->set_request_id(uid);
                response->set_success(false);
                response->set_errmsg(err_msg);
            };
            // 1. 从请求中取出用户 ID 与新的昵称
            auto user_id = request->user_id();
            auto new_name = request->nickname();
            // 2. 判断昵称格式是否正确
            if (check_name(new_name) == false)
            {
                ERROR_LOG("{}：设置的名称格式不正确", request->request_id());
                err_response(request->request_id(), "名称格式不正确");
                return;
            }
            // 3. 从数据库通过用户 ID 进行用户信息查询，判断用户是否存在
            auto user = _mysql_user->select_by_user_id(user_id);
            if (!user)
            {
                ERROR_LOG("{}：用户信息不存在", request->request_id());
                err_response(request->request_id(), "用户信息不存在");
                return;
            }
            // 4. 将新的昵称更新到数据库中
            user->nickname(new_name);
            _mysql_user->update(user);
            // 5. 更新 ES 服务器中用户信息
            _es_user->appendData(user->user_id(), user->phone(), new_name, user->description(), user->avatar_id());
            // 6. 组织响应，返回更新成功与否
            response->set_request_id(request->request_id());
            response->set_success(true);
        }
        virtual void SetUserDescription(::google::protobuf::RpcController *controller,
                                        const ::tew_im::SetUserDescriptionReq *request,
                                        ::tew_im::SetUserDescriptionRsp *response,
                                        ::google::protobuf::Closure *done)
        {
            DEBUG_LOG("收到用户签名设置请求！");
            brpc::ClosureGuard rpc_guard(done);
            // 先构建一个错误响应回调函数
            auto err_response = [this, response](const std::string &uid, const std::string &err_msg)
            {
                response->set_request_id(uid);
                response->set_success(false);
                response->set_errmsg(err_msg);
            };
            // 1. 从请求中取出用户 ID 与新的昵称
            auto user_id = request->user_id();
            auto new_desc = request->description();
            // 3. 从数据库通过用户 ID 进行用户信息查询，判断用户是否存在
            auto user = _mysql_user->select_by_user_id(user_id);
            if (!user)
            {
                ERROR_LOG("{}：用户信息不存在", request->request_id());
                err_response(request->request_id(), "用户信息不存在");
                return;
            }
            // 4. 将新描述更新到数据库中
            user->description(new_desc);
            _mysql_user->update(user);
            // 5. 更新 ES 服务器中用户信息
            _es_user->appendData(user->user_id(), user->phone(), user->nickname(), new_desc, user->avatar_id());
            // 6. 组织响应，返回更新成功与否
            response->set_request_id(request->request_id());
            response->set_success(true);
        }
        virtual void SetUserPhoneNumber(::google::protobuf::RpcController *controller,
                                        const ::tew_im::SetUserPhoneNumberReq *request,
                                        ::tew_im::SetUserPhoneNumberRsp *response,
                                        ::google::protobuf::Closure *done)
        {
            DEBUG_LOG("收到用户手机号设置请求！");
            brpc::ClosureGuard rpc_guard(done);
        }

    private:
        ESUser::ptr _es_user;
        UserTable::ptr _mysql_user;
        Session::ptr _redis_session;
        Status::ptr _redis_status;
        Codes::ptr _redis_codes;
        // 这边是rpc调用客户端相关对象
        std::string _file_service_name;
        ServiceManager::ptr _mm_channels;
    };

    class UserServer
    {
    public:
        using ptr = std::shared_ptr<UserServer>;
        UserServer(const Discovery::ptr service_discoverer,
                   const Registry::ptr &reg_client,
                   const std::shared_ptr<elasticlient::Client> &es_client,
                   const std::shared_ptr<odb::core::database> &mysql_client,
                   std::shared_ptr<sw::redis::Redis> &redis_client,
                   const std::shared_ptr<brpc::Server> &server) : _service_discoverer(service_discoverer),
                                                                  _registry_client(reg_client),
                                                                  _es_client(es_client),
                                                                  _mysql_client(mysql_client),
                                                                  _redis_client(redis_client),
                                                                  _rpc_server(server) {}
        ~UserServer() {}
        // 搭建RPC服务器，并启动服务器
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }

    private:
        Discovery::ptr _service_discoverer;
        Registry::ptr _registry_client;
        std::shared_ptr<elasticlient::Client> _es_client;
        std::shared_ptr<odb::core::database> _mysql_client;
        std::shared_ptr<sw::redis::Redis> _redis_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };

    class UserServerBuilder
    {
    public:
        // 构造es客户端对象
        void make_es_object(const std::vector<std::string> host_list)
        {
            _es_client = ESClientFactory::create(host_list);
        }
        // 构造mysql客户端对象
        void make_mysql_object(
            const std::string &user,
            const std::string &pswd,
            const std::string &host,
            const std::string &db,
            const std::string &cset,
            int port,
            int conn_pool_count)
        {
            _mysql_client = DataBaseFactory::create(user, pswd, db, host, port, cset, conn_pool_count);
        }

        // 构造redis客户端对象
        void make_redis_object(const std::string &host,
                               int port,
                               int db,
                               bool keep_alive)
        {
            _redis_client = RedisClientFactory::create(host, port, db, keep_alive);
        }
        // 用于构造服务发现客户端&信道管理对象
        void make_discovery_object(const std::string &reg_host,
                                   const std::string &base_service_name,
                                   const std::string &file_service_name)
        {
            _file_service_name = file_service_name;
            _mm_channels = std::make_shared<ServiceManager>();
            _mm_channels->Declared(file_service_name);
            cout << "用户子服务关心的服务是：" << file_service_name << endl;
            DEBUG_LOG("设置文件子服务为需添加管理的子服务：{}", file_service_name);
            auto put_cb = std::bind(&ServiceManager::ServiceOnline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::ServiceOffline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            _service_discoverer = std::make_shared<Discovery>(reg_host, put_cb, del_cb);
            _service_discoverer->discovery(base_service_name);
        }
        // 用于构造服务注册客户端对象
        void make_registry_object(const std::string &reg_host,
                                  const std::string &service_name,
                                  const std::string &access_host)
        {
            _registry_client = std::make_shared<Registry>(reg_host);
            _registry_client->registry(service_name, access_host);
        }
        void make_rpc_server(uint16_t port, int32_t timeout, uint8_t num_threads)
        {
            if (!_es_client)
            {
                ERROR_LOG("还未初始化ES搜索引擎模块！");
                abort();
            }
            if (!_mysql_client)
            {
                ERROR_LOG("还未初始化Mysql数据库模块！");
                abort();
            }
            if (!_redis_client)
            {
                ERROR_LOG("还未初始化Redis数据库模块！");
                abort();
            }
            if (!_mm_channels)
            {
                ERROR_LOG("还未初始化信道管理模块！");
                abort();
            }
            _rpc_server = std::make_shared<brpc::Server>();

            UserServiceImpl *user_service = new UserServiceImpl(_es_client,
                                                                _mysql_client,
                                                                _redis_client,
                                                                _mm_channels,
                                                                _file_service_name);
            int ret = _rpc_server->AddService(user_service, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            if (ret == -1)
            {
                ERROR_LOG("添加Rpc服务失败！");
                abort();
            }
            brpc::ServerOptions options;
            options.idle_timeout_sec = timeout;
            options.num_threads = num_threads;
            ret = _rpc_server->Start(port, &options);
            if (ret == -1)
            {
                ERROR_LOG("服务启动失败！");
                abort();
            }
        }
        // 构造RPC服务器对象
        UserServer::ptr build()
        {
            if (!_service_discoverer)
            {
                ERROR_LOG("还未初始化服务发现模块！");
                abort();
            }
            if (!_registry_client)
            {
                ERROR_LOG("还未初始化服务注册模块！");
                abort();
            }
            if (!_rpc_server)
            {
                ERROR_LOG("还未初始化RPC服务器模块！");
                abort();
            }
            UserServer::ptr server = std::make_shared<UserServer>(_service_discoverer,
                                                                  _registry_client,
                                                                  _es_client,
                                                                  _mysql_client,
                                                                  _redis_client,
                                                                  _rpc_server);
            return server;
        }

    private:
        Registry::ptr _registry_client;

        std::shared_ptr<elasticlient::Client> _es_client;
        std::shared_ptr<odb::core::database> _mysql_client;
        std::shared_ptr<sw::redis::Redis> _redis_client;

        std::string _file_service_name;
        ServiceManager::ptr _mm_channels;
        Discovery::ptr _service_discoverer;

        std::shared_ptr<brpc::Server> _rpc_server;
    };
}
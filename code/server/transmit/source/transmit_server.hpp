// 实现语音识别子服务
#include <brpc/server.h>
#include <butil/logging.h>

#include "mysql_chat_session_member.hpp" // mysql数据管理客户端封装
#include "etcd.hpp"                      // 服务注册模块封装
#include "logger.hpp"                    // 日志模块封装
#include "utils.hpp"                     // 基础工具接口
#include "rabbitmq.hpp"
#include "channel.hpp" // 信道管理模块封装

#include "chat_session_member.hxx"
#include "chat_session_member-odb.hxx"

#include "transmit.pb.h" // protobuf框架代码
#include "base.pb.h"     // protobuf框架代码
#include "user.pb.h"

namespace tew_im
{
    class MsgTransmitServiceImpl : public tew_im::MsgTransmitService
    {
    public:
        MsgTransmitServiceImpl(
            const std::shared_ptr<odb::core::database> &mysql_client,
            const MQClient::ptr &mq_client,
            const std::string &exchange_name,
            const std::string &routing_key,
            const ServiceManager::ptr &channel_manager,
            const std::string &user_service_name) : _chat_session_table(std::make_shared<ChatSessionMemberTable>(std::dynamic_pointer_cast<odb::mysql::database>(mysql_client))),
                                                    _mq_client(mq_client),
                                                    _user_service_name(user_service_name),
                                                    _mm_channels(channel_manager),
                                                    _exchange_name(exchange_name),
                                                    _routing_key(routing_key)
        {
            // 消息转发子服务要干什么呢？
            // 1.获取到请求后，对请求中的信息进行组织，组织出(发送者信息-调用用户子服务，所属会话，时间，消息id，消息内容)
            // 2.组织完毕后，获取发送客户端列表--根据所属聊天会话id，对聊天会话成员表进行查询
            // 3.最后，将组织完毕的信息，发布到消息队列中，等待消息存储子服务进行持久化存储
        }
        void GetTransmitTarget(google::protobuf::RpcController *controller,
                               const ::tew_im::NewMessageReq *request,
                               ::tew_im::GetTransmitTargetRsp *response,
                               ::google::protobuf::Closure *done)
        {
            // 先构建一个错误响应回调函数
            auto err_response = [this, response](const std::string &uid, const std::string &err_msg)
            {
                response->set_request_id(uid);
                response->set_success(false);
                response->set_errmsg(err_msg);
            };
            brpc::ClosureGuard rpc_guard(done);
            // 1. 从请求中取出消息内容，会话 ID， 用户 ID
            std::string chat_session_id = request->chat_session_id();
            std::string uid = request->user_id();
            const MessageContent &msgcontent = request->message();
            // 2. 根据用户 ID 从用户子服务获取当前发送者用户信息
            auto channel = _mm_channels->Choose(_user_service_name);
            if (!channel)
            {
                ERROR_LOG("{}：用户子服务信道对象不存在:{}", request->request_id(), _user_service_name);
                err_response(request->request_id(), "用户子服务信道不存在");
                return;
            }
            tew_im::UserService_Stub stub(channel.get());
            tew_im::GetUserInfoReq req;
            tew_im::GetUserInfoRsp rsp;
            brpc::Controller cntl;
            req.set_request_id(request->request_id());
            req.set_user_id(uid);
            stub.GetUserInfo(&cntl, &req, &rsp, nullptr);
            if (cntl.Failed() == true || rsp.success() == false)
            {
                ERROR_LOG("{}：调用用户子服务失败:{}", request->request_id(), cntl.ErrorText());
                err_response(request->request_id(), "获取用户信息失败");
                return;
            }
            // 3. 根据消息内容构造完成的消息结构（分配消息 ID，填充发送者信息，填充消息产
            // 生时间）
            MessageInfo msginfo;
            msginfo.set_message_id(Uuid());
            msginfo.set_chat_session_id(chat_session_id);
            msginfo.set_timestamp(time(nullptr));
            msginfo.mutable_sender()->CopyFrom(rsp.user_info());
            msginfo.mutable_message()->CopyFrom(msgcontent);
            // 4. 将消息序列化后发布到 MQ 消息队列中，让消息存储子服务对消息进行持久化存储（发布消息需要指定交换机名称）
            bool ret = _mq_client->publish(_exchange_name, msginfo.SerializeAsString(), _routing_key);
            if (ret == false)
            {
                ERROR_LOG("{}：持久化消息发布到{}失败", request->request_id(), _exchange_name);
                err_response(request->request_id(), "消息发布失败，消息无法持久化");
                return;
            }
            // 5. 从数据库获取目标会话所有成员 ID
            auto target_list = _chat_session_table->members_id(chat_session_id);
            // 6. 组织响应（完整消息+目标用户 ID），发送给网关，告知网关该将消息发送给谁。
            response->set_request_id(request->request_id());
            response->set_success(true);
            response->mutable_message()->CopyFrom(msginfo);
            for (auto &uid : target_list)
            {
                response->add_target_id_list(uid);
            }
        }
        ~MsgTransmitServiceImpl() {}

    private:
        // 需要调用用户子服务，这边是rpc调用客户端相关对象
        std::string _user_service_name;
        ServiceManager::ptr _mm_channels;
        // 需要查询聊天会话成员表，获取转发客户端列表
        ChatSessionMemberTable::ptr _chat_session_table;
        // 需要将组织完毕的消息发布到消息队列中
        MQClient::ptr _mq_client;
        std::string _exchange_name;
        std::string _routing_key;
    };

    class TransmitServer
    {
    public:
        using ptr = std::shared_ptr<TransmitServer>;
        TransmitServer(const Discovery::ptr service_discoverer,
                       const Registry::ptr &reg_client,
                       const std::shared_ptr<odb::core::database> &mysql_client,
                       const MQClient::ptr mq_client,
                       const std::shared_ptr<brpc::Server> &server) : _service_discoverer(service_discoverer),
                                                                      _registry_client(reg_client),
                                                                      _mq_client(mq_client),
                                                                      _mysql_client(mysql_client),
                                                                      _rpc_server(server) {}
        ~TransmitServer() {}
        // 搭建RPC服务器，并启动服务器
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }

    private:
        Discovery::ptr _service_discoverer;
        Registry::ptr _registry_client;
        std::shared_ptr<odb::core::database> _mysql_client;
        std::shared_ptr<brpc::Server> _rpc_server;
        MQClient::ptr _mq_client;
    };

    class TransmitServerBuilder
    {
    public:
        // 构造mq客户端对象
        void make_mq_object(const std::string &user,
                            const std::string &paswd,
                            const std::string &host,
                            const std::string &exchange_name,
                            const std::string &queue_name,
                            const std::string &routing_key)
        {
            _mq_client = std::make_shared<MQClient>(user, paswd, host);
            _exchange_name = exchange_name;
            _routing_key = routing_key;
            _mq_client->delcarcomponent(exchange_name, queue_name, routing_key);
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

        // 用于构造服务发现客户端&信道管理对象
        void make_discovery_object(const std::string &reg_host,
                                   const std::string &base_service_name,
                                   const std::string &user_service_name)
        {
            _user_service_name = user_service_name;
            _mm_channels = std::make_shared<ServiceManager>();
            _mm_channels->Declared(_user_service_name);
            cout << "消息转发子服务关心的服务是：" << _user_service_name << endl;
            DEBUG_LOG("设置用户子服务为需添加管理的子服务：{}", _user_service_name);
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
            if (!_mq_client)
            {
                ERROR_LOG("还未初始化mq消息队列模块！");
                abort();
            }
            if (!_mysql_client)
            {
                ERROR_LOG("还未初始化Mysql数据库模块！");
                abort();
            }
            if (!_mm_channels)
            {
                ERROR_LOG("还未初始化信道管理模块！");
                abort();
            }
            _rpc_server = std::make_shared<brpc::Server>();
            MsgTransmitServiceImpl *transmit_service = new MsgTransmitServiceImpl(_mysql_client,
                                                                                  _mq_client,
                                                                                  _exchange_name,
                                                                                  _routing_key,
                                                                                  _mm_channels,
                                                                                  _user_service_name);
            int ret = _rpc_server->AddService(transmit_service, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
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
        TransmitServer::ptr build()
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
            TransmitServer::ptr server = std::make_shared<TransmitServer>(_service_discoverer,
                                                                          _registry_client,
                                                                          _mysql_client,
                                                                          _mq_client,
                                                                          _rpc_server);
            return server;
        }

    private:
        std::string _user_service_name;
        Discovery::ptr _service_discoverer;
        ServiceManager::ptr _mm_channels;

        Registry::ptr _registry_client;
        std::shared_ptr<odb::core::database> _mysql_client;
        MQClient::ptr _mq_client;
        std::string _exchange_name;
        std::string _routing_key;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
}
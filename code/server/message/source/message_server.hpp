// 手动声明缺失的枚举，解决编译报错
#ifndef NET_ASYNC_STATUS_DEFINED
#define NET_ASYNC_STATUS_DEFINED
enum net_async_status
{
    NET_ASYNC_COMPLETE = 0,    // 异步操作完成
    NET_ASYNC_IN_PROGRESS = 1, // 异步操作中
    NET_ASYNC_ERROR = 2        // 异步操作错误
};
#endif
// 实现语音识别子服务
#include <brpc/server.h>
#include <butil/logging.h>

#include "data_es.hpp"             // es数据管理客户端封装
#include "mysql_message_store.hpp" // mysql数据管理客户端封装
#include "etcd.hpp"                // 服务注册模块封装
#include "logger.hpp"              // 日志模块封装
#include "utils.hpp"               // 基础工具接口
#include "channel.hpp"             // 信道管理模块封装
#include "rabbitmq.hpp"

#include "message.pb.h" // protobuf框架代码
#include "base.pb.h"    // protobuf框架代码
#include "file.pb.h"    // protobuf框架代码
#include "user.pb.h"    // protobuf框架代码

namespace tew_im
{
    class MessageServiceImpl : public tew_im::MsgStorageService
    {
    public:
        MessageServiceImpl(
            const std::shared_ptr<elasticlient::Client> &es_client,
            const std::shared_ptr<odb::core::database> &mysql_client,
            const ServiceManager::ptr &channel_manager,
            const std::string &file_service_name,
            const std::string &user_service_name) : _es_message(std::make_shared<ESMessage>(es_client)),
                                                    _mysql_message(std::make_shared<MessageStoreTable>(std::dynamic_pointer_cast<odb::mysql::database>(mysql_client))),
                                                    _file_service_name(file_service_name),
                                                    _user_service_name(user_service_name),
                                                    _mm_channels(channel_manager)
        {
            _es_message->createIndex();
        }
        ~MessageServiceImpl() {}
        virtual void GetHistoryMsg(::google::protobuf::RpcController *controller,
                                   const ::tew_im::GetHistoryMsgReq *request,
                                   ::tew_im::GetHistoryMsgRsp *response,
                                   ::google::protobuf::Closure *done)
        {
            brpc::ClosureGuard rpc_guard(done);
            auto err_response = [this, response](const std::string &rid,
                                                 const std::string &errmsg) -> void
            {
                response->set_request_id(rid);
                response->set_success(false);
                response->set_errmsg(errmsg);
                return;
            };
            // 1. 提取关键要素：会话ID，起始时间，结束时间
            std::string rid = request->request_id();
            std::string chat_ssid = request->chat_session_id();
            boost::posix_time::ptime stime = boost::posix_time::from_time_t(request->start_time());
            boost::posix_time::ptime etime = boost::posix_time::from_time_t(request->over_time());
            // 2. 从数据库中进行消息查询
            auto msg_lists = _mysql_message->range(chat_ssid, stime, etime);
            if (msg_lists.empty())
            {
                response->set_request_id(rid);
                response->set_success(true);
                return;
            }
            // 3. 统计所有文件类型消息的文件ID，并从文件子服务进行批量文件下载
            std::unordered_set<std::string> file_id_lists;
            for (auto &msg : msg_lists)
            {
                if (msg.file_id().empty())
                    continue;
                DEBUG_LOG("需要下载的文件ID： {}", msg.file_id());
                file_id_lists.insert(msg.file_id());
            }
            std::unordered_map<std::string, std::string> file_data_lists;
            bool ret = _GetFile(rid, file_id_lists, file_data_lists);
            if (ret == false)
            {
                ERROR_LOG("{} 批量文件数据下载失败！", rid);
                return err_response(rid, "批量文件数据下载失败!");
            }
            // 4. 统计所有消息的发送者用户ID，从用户子服务进行批量用户信息获取
            std::unordered_set<std::string> user_id_lists; // {猪爸爸吧， 祝妈妈，猪爸爸吧，祝爸爸}
            for (auto &msg : msg_lists)
            {
                user_id_lists.insert(msg.sender_id());
            }
            std::unordered_map<std::string, UserInfo> user_lists;
            ret = _GetUser(rid, user_id_lists, user_lists);
            if (ret == false)
            {
                ERROR_LOG("{} 批量用户数据获取失败！", rid);
                return err_response(rid, "批量用户数据获取失败!");
            }
            // 5. 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            for (auto &msg : msg_lists)
            {
                auto message_info = response->add_msg_list();
                message_info->set_message_id(msg.message_id());
                message_info->set_chat_session_id(msg.chat_session_id());
                message_info->set_timestamp(boost::posix_time::to_time_t(msg.create_time()));
                message_info->mutable_sender()->CopyFrom(user_lists[msg.sender_id()]);
                switch (msg.type())
                {
                case MessageType::STRING:
                    message_info->mutable_message()->set_message_type(MessageType::STRING);
                    message_info->mutable_message()->mutable_string_message()->set_content(msg.msg_content());
                    break;
                case MessageType::IMAGE:
                    message_info->mutable_message()->set_message_type(MessageType::IMAGE);
                    message_info->mutable_message()->mutable_image_message()->set_file_id(msg.file_id());
                    message_info->mutable_message()->mutable_image_message()->set_image_content(file_data_lists[msg.file_id()]);
                    break;
                case MessageType::FILE:
                    message_info->mutable_message()->set_message_type(MessageType::FILE);
                    message_info->mutable_message()->mutable_file_message()->set_file_id(msg.file_id());
                    message_info->mutable_message()->mutable_file_message()->set_file_size(msg.file_size());
                    message_info->mutable_message()->mutable_file_message()->set_file_name(msg.file_name());
                    message_info->mutable_message()->mutable_file_message()->set_file_contents(file_data_lists[msg.file_id()]);
                    break;
                case MessageType::SPEECH:
                    message_info->mutable_message()->set_message_type(MessageType::SPEECH);
                    message_info->mutable_message()->mutable_speech_message()->set_file_id(msg.file_id());
                    message_info->mutable_message()->mutable_speech_message()->set_file_contents(file_data_lists[msg.file_id()]);
                    break;
                default:
                    ERROR_LOG("消息类型错误！！");
                    return;
                }
            }
            return;
        }
        virtual void GetRecentMsg(::google::protobuf::RpcController *controller,
                                  const ::tew_im::GetRecentMsgReq *request,
                                  ::tew_im::GetRecentMsgRsp *response,
                                  ::google::protobuf::Closure *done)
        {
            brpc::ClosureGuard rpc_guard(done);
            auto err_response = [this, response](const std::string &rid,
                                                 const std::string &errmsg) -> void
            {
                response->set_request_id(rid);
                response->set_success(false);
                response->set_errmsg(errmsg);
                return;
            };
            // 1. 提取请求中的关键要素：请求ID，会话ID，要获取的消息数量
            std::string rid = request->request_id();
            std::string chat_ssid = request->chat_session_id();
            int msg_count = request->msg_count();
            // 2. 从数据库，获取最近的消息元信息
            auto msg_lists = _mysql_message->recent(chat_ssid, msg_count);
            if (msg_lists.empty())
            {
                response->set_request_id(rid);
                response->set_success(true);
                return;
            }
            // 3. 统计所有消息中文件类型消息的文件ID列表，从文件子服务下载文件
            std::unordered_set<std::string> file_id_lists;
            for (auto &msg : msg_lists)
            {
                if (msg.file_id().empty())
                    continue;
                DEBUG_LOG("需要下载的文件ID: {}", msg.file_id());
                file_id_lists.insert(msg.file_id());
            }
            std::unordered_map<std::string, std::string> file_data_lists;
            bool ret = _GetFile(rid, file_id_lists, file_data_lists);
            if (ret == false)
            {
                ERROR_LOG("{} 批量文件数据下载失败！", rid);
                return err_response(rid, "批量文件数据下载失败!");
            }
            // 4. 统计所有消息的发送者用户ID，从用户子服务进行批量用户信息获取
            std::unordered_set<std::string> user_id_lists;
            for (auto &msg : msg_lists)
            {
                user_id_lists.insert(msg.sender_id());
            }
            std::unordered_map<std::string, UserInfo> user_lists;
            ret = _GetUser(rid, user_id_lists, user_lists);
            if (ret == false)
            {
                ERROR_LOG("{} 批量用户数据获取失败！", rid);
                return err_response(rid, "批量用户数据获取失败!");
            }
            // 5. 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            for (auto &msg : msg_lists)
            {
                auto message_info = response->add_msg_list();
                message_info->set_message_id(msg.message_id());
                message_info->set_chat_session_id(msg.chat_session_id());
                message_info->set_timestamp(boost::posix_time::to_time_t(msg.create_time()));
                message_info->mutable_sender()->CopyFrom(user_lists[msg.sender_id()]);
                switch (msg.type())
                {
                case MessageType::STRING:
                    message_info->mutable_message()->set_message_type(MessageType::STRING);
                    message_info->mutable_message()->mutable_string_message()->set_content(msg.msg_content());
                    break;
                case MessageType::IMAGE:
                    message_info->mutable_message()->set_message_type(MessageType::IMAGE);
                    message_info->mutable_message()->mutable_image_message()->set_file_id(msg.file_id());
                    message_info->mutable_message()->mutable_image_message()->set_image_content(file_data_lists[msg.file_id()]);
                    break;
                case MessageType::FILE:
                    message_info->mutable_message()->set_message_type(MessageType::FILE);
                    message_info->mutable_message()->mutable_file_message()->set_file_id(msg.file_id());
                    message_info->mutable_message()->mutable_file_message()->set_file_size(msg.file_size());
                    message_info->mutable_message()->mutable_file_message()->set_file_name(msg.file_name());
                    message_info->mutable_message()->mutable_file_message()->set_file_contents(file_data_lists[msg.file_id()]);
                    break;
                case MessageType::SPEECH:
                    message_info->mutable_message()->set_message_type(MessageType::SPEECH);
                    message_info->mutable_message()->mutable_speech_message()->set_file_id(msg.file_id());
                    message_info->mutable_message()->mutable_speech_message()->set_file_contents(file_data_lists[msg.file_id()]);
                    break;
                default:
                    ERROR_LOG("消息类型错误！！");
                    return;
                }
            }
            return;
        }
        virtual void MsgSearch(::google::protobuf::RpcController *controller,
                               const ::tew_im::MsgSearchReq *request,
                               ::tew_im::MsgSearchRsp *response,
                               ::google::protobuf::Closure *done)
        {
            brpc::ClosureGuard rpc_guard(done);
            auto err_response = [this, response](const std::string &rid,
                                                 const std::string &errmsg) -> void
            {
                response->set_request_id(rid);
                response->set_success(false);
                response->set_errmsg(errmsg);
                return;
            };
            // 关键字的消息搜索--只针对文本消息
            // 1. 从请求中提取关键要素：请求ID，会话ID, 关键字
            std::string rid = request->request_id();
            std::string chat_ssid = request->chat_session_id();
            std::string skey = request->search_key();
            // 2. 从ES搜索引擎中进行关键字消息搜索，得到消息列表
            auto msg_lists = _es_message->search(skey, chat_ssid);
            if (msg_lists.empty())
            {
                response->set_request_id(rid);
                response->set_success(true);
                return;
            }
            // 3. 组织所有消息的用户ID，从用户子服务获取用户信息
            std::unordered_set<std::string> user_id_lists;
            for (auto &msg : msg_lists)
            {
                user_id_lists.insert(msg.sender_id());
            }
            std::unordered_map<std::string, UserInfo> user_lists;
            bool ret = _GetUser(rid, user_id_lists, user_lists);
            if (ret == false)
            {
                ERROR_LOG("{} 批量用户数据获取失败！", rid);
                return err_response(rid, "批量用户数据获取失败!");
            }
            // 4. 组织响应
            response->set_request_id(rid);
            response->set_success(true);
            for (auto &msg : msg_lists)
            {
                auto message_info = response->add_msg_list();
                message_info->set_message_id(msg.message_id());
                message_info->set_chat_session_id(msg.chat_session_id());
                message_info->set_timestamp(boost::posix_time::to_time_t(msg.create_time()));
                message_info->mutable_sender()->CopyFrom(user_lists[msg.sender_id()]);
                message_info->mutable_message()->set_message_type(MessageType::STRING);
                message_info->mutable_message()->mutable_string_message()->set_content(msg.msg_content());
            }
            return;
        }
        void onMessage(const char *body, size_t sz)
        {
            DEBUG_LOG("收到新消息，进行存储处理！");
            // 1. 取出序列化的消息内容，进行反序列化
            tew_im::MessageInfo message;
            bool ret = message.ParseFromArray(body, sz);
            if (ret == false)
            {
                ERROR_LOG("对消费到的消息进行反序列化失败！");
                return;
            }
            // 2. 根据不同的消息类型进行不同的处理
            std::string file_id, file_name, content;
            int64_t file_size;
            switch (message.message().message_type())
            {
            //  1. 如果是一个文本类型消息，取元信息存储到ES中
            case MessageType::STRING:
                content = message.message().string_message().content();
                ret = _es_message->appendData(
                    message.sender().user_id(),
                    message.message_id(),
                    message.timestamp(),
                    message.chat_session_id(),
                    content);
                if (ret == false)
                {
                    ERROR_LOG("文本消息向存储引擎进行存储失败！");
                    return;
                }
                break;
            //  2. 如果是一个图片/语音/文件消息，则取出数据存储到文件子服务中，并获取文件ID
            case MessageType::IMAGE:
            {
                auto &msg = message.message().image_message();
                ret = _PutFile("", msg.image_content(), msg.image_content().size(), file_id);
                if (ret == false)
                {
                    ERROR_LOG("上传图片到文件子服务失败！");
                    return;
                }
            }
            break;
            case MessageType::FILE:
            {
                auto &msg = message.message().file_message();
                file_name = msg.file_name();
                file_size = msg.file_size();
                ret = _PutFile(file_name, msg.file_contents(), file_size, file_id);
                if (ret == false)
                {
                    ERROR_LOG("上传文件到文件子服务失败！");
                    return;
                }
            }
            break;
            case MessageType::SPEECH:
            {
                auto &msg = message.message().speech_message();
                ret = _PutFile("", msg.file_contents(), msg.file_contents().size(), file_id);
                if (ret == false)
                {
                    ERROR_LOG("上传语音到文件子服务失败！");
                    return;
                }
            }
            break;
            default:
                ERROR_LOG("消息类型错误！");
                return;
            }
            // 3. 提取消息的元信息，存储到mysql数据库中
            tew_im::MessageStore msg(message.message_id(),
                                     message.chat_session_id(),
                                     message.sender().user_id(),
                                     boost::posix_time::from_time_t(message.timestamp()),
                                     message.message().message_type());
            msg.msg_content(content);
            msg.file_id(file_id);
            msg.file_name(file_name);
            msg.file_size(file_size);
            ret = _mysql_message->append(msg);
            if (ret == false)
            {
                ERROR_LOG("向数据库插入新消息失败！");
                return;
            }
        }

    private:
        bool _GetUser(const std::string &rid,
                      const std::unordered_set<std::string> &user_id_lists,
                      std::unordered_map<std::string, UserInfo> &user_lists)
        {
            auto channel = _mm_channels->Choose(_user_service_name);
            if (!channel)
            {
                ERROR_LOG("{} 没有可供访问的用户子服务节点！", _user_service_name);
                return false;
            }
            UserService_Stub stub(channel.get());
            GetMultiUserInfoReq req;
            GetMultiUserInfoRsp rsp;
            req.set_request_id(rid);
            for (auto &id : user_id_lists)
            {
                req.add_users_id(id);
            }
            brpc::Controller cntl;
            stub.GetMultiUserInfo(&cntl, &req, &rsp, nullptr);
            if (cntl.Failed() == true || rsp.success() == false)
            {
                ERROR_LOG("用户子服务调用失败：{}！", cntl.ErrorText());
                return false;
            }
            auto &umap = rsp.users_info();
            for (auto it = umap.begin(); it != umap.end(); ++it)
            {
                user_lists.insert(std::make_pair(it->first, it->second));
            }
            return true;
        }
        bool _GetFile(const std::string &rid,
                      const std::unordered_set<std::string> &file_id_lists,
                      std::unordered_map<std::string, std::string> &file_data_lists)
        {
            auto channel = _mm_channels->Choose(_file_service_name);
            if (!channel)
            {
                ERROR_LOG("{} 没有可供访问的文件子服务节点！", _file_service_name);
                return false;
            }
            FileService_Stub stub(channel.get());
            GetMultiFileReq req;
            GetMultiFileRsp rsp;
            req.set_request_id(rid);
            for (auto &id : file_id_lists)
            {
                req.add_file_id_list(id);
            }
            brpc::Controller cntl;
            stub.GetMultiFile(&cntl, &req, &rsp, nullptr);
            if (cntl.Failed() == true || rsp.success() == false)
            {
                ERROR_LOG("文件子服务调用失败：{}！", cntl.ErrorText());
                return false;
            }
            auto &fmap = rsp.file_data();
            for (auto it = fmap.begin(); it != fmap.end(); ++it)
            {
                file_data_lists.insert(std::make_pair(it->first, it->second.file_content()));
            }
            return true;
        }
        bool _PutFile(const std::string &filename,
                      const std::string &body,
                      const int64_t fsize,
                      std::string &file_id)
        {
            // 实现文件数据的上传
            auto channel = _mm_channels->Choose(_file_service_name);
            if (!channel)
            {
                ERROR_LOG("{} 没有可供访问的文件子服务节点！", _file_service_name);
                return false;
            }
            FileService_Stub stub(channel.get());
            PutSingleFileReq req;
            PutSingleFileRsp rsp;
            req.mutable_file_data()->set_file_name(filename);
            req.mutable_file_data()->set_file_size(fsize);
            req.mutable_file_data()->set_file_content(body);
            brpc::Controller cntl;
            stub.PutSingleFile(&cntl, &req, &rsp, nullptr);
            if (cntl.Failed() == true || rsp.success() == false)
            {
                ERROR_LOG("文件子服务调用失败：{}！", cntl.ErrorText());
                return false;
            }
            file_id = rsp.file_info().file_id();
            return true;
        }

    private:
        ESMessage::ptr _es_message;
        MessageStoreTable::ptr _mysql_message;
        // 这边是rpc调用客户端相关对象
        std::string _user_service_name;
        std::string _file_service_name;
        ServiceManager::ptr _mm_channels;
    };

    class MessageServer
    {
    public:
        using ptr = std::shared_ptr<MessageServer>;
        MessageServer(const MQClient::ptr &mq_client,
                      const Discovery::ptr service_discoverer,
                      const Registry::ptr &reg_client,
                      const std::shared_ptr<elasticlient::Client> &es_client,
                      const std::shared_ptr<odb::core::database> &mysql_client,
                      const std::shared_ptr<brpc::Server> &server) : _mq_client(mq_client),
                                                                     _service_discoverer(service_discoverer),
                                                                     _registry_client(reg_client),
                                                                     _es_client(es_client),
                                                                     _mysql_client(mysql_client),
                                                                     _rpc_server(server) {}
        ~MessageServer() {}
        // 搭建RPC服务器，并启动服务器
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }

    private:
        Discovery::ptr _service_discoverer;
        Registry::ptr _registry_client;
        MQClient::ptr _mq_client;
        std::shared_ptr<elasticlient::Client> _es_client;
        std::shared_ptr<odb::core::database> _mysql_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };

    class MessageServerBuilder
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
        // 用于构造服务发现客户端&信道管理对象
        void make_discovery_object(const std::string &reg_host,
                                   const std::string &base_service_name,
                                   const std::string &file_service_name,
                                   const std::string &user_service_name)
        {
            _user_service_name = user_service_name;
            _file_service_name = file_service_name;
            _mm_channels = std::make_shared<ServiceManager>();
            _mm_channels->Declared(file_service_name);
            _mm_channels->Declared(user_service_name);
            DEBUG_LOG("设置文件子服务为需添加管理的子服务：{}", file_service_name);
            auto put_cb = std::bind(&ServiceManager::ServiceOnline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
            auto del_cb = std::bind(&ServiceManager::ServiceOnline, _mm_channels.get(), std::placeholders::_1, std::placeholders::_2);
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
        // 用于构造消息队列客户端对象
        void make_mq_object(const std::string &user,
                            const std::string &passwd,
                            const std::string &host,
                            const std::string &exchange_name,
                            const std::string &queue_name,
                            const std::string &binding_key)
        {
            _exchange_name = exchange_name;
            _queue_name = queue_name;
            _mq_client = std::make_shared<MQClient>(user, passwd, host);
            _mq_client->delcarcomponent(exchange_name, queue_name, binding_key);
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
            if (!_mm_channels)
            {
                ERROR_LOG("还未初始化信道管理模块！");
                abort();
            }
            _rpc_server = std::make_shared<brpc::Server>();

            MessageServiceImpl *msg_service = new MessageServiceImpl(_es_client,
                                                                     _mysql_client, _mm_channels, _file_service_name, _user_service_name);
            int ret = _rpc_server->AddService(msg_service,
                                              brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
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

            auto callback = std::bind(&MessageServiceImpl::onMessage, msg_service,
                                      std::placeholders::_1, std::placeholders::_2);
            _mq_client->consume(_queue_name, callback);
        }
        // 构造RPC服务器对象
        MessageServer::ptr build()
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

            MessageServer::ptr server = std::make_shared<MessageServer>(
                _mq_client, _service_discoverer, _registry_client,
                _es_client, _mysql_client, _rpc_server);
            return server;
        }

    private:
        Registry::ptr _registry_client;

        std::shared_ptr<elasticlient::Client> _es_client;
        std::shared_ptr<odb::core::database> _mysql_client;

        std::string _user_service_name;
        std::string _file_service_name;
        ServiceManager::ptr _mm_channels;
        Discovery::ptr _service_discoverer;

        std::string _exchange_name;
        std::string _queue_name;
        MQClient::ptr _mq_client;

        std::shared_ptr<brpc::Server> _rpc_server;
    };
}
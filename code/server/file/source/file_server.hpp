/*文件存储子服务模块可以分为三部分：
1.先实现具体的功能接口，也就是重写虚函数，实现文件存储业务逻辑
2.封装实现文件存储服务器模块
3.再实现文件存储服务器的建造者模块*/

#include <brpc/server.h>
#include <butil/logging.h>
#include "file.pb.h" //protobuf生成的rpc框架代码
#include "base.pb.h"
#include "logger.hpp" //日志模块封装
#include "etcd.hpp"   //注册中心模块封装
#include "utils.hpp"
namespace tew_im
{
    /*语音识别子服务业务逻辑：
    1.接收请求，获取请求中的语音数据speech_content  2.调用ASRClient接口，获取结果 3.组织响应发送回去
    */
    class FileSeriveImpl : public tew_im::FileService
    {
    public:
        FileSeriveImpl() {}
        ~FileSeriveImpl() {}
        // 单文件上传
        void PutSingleFile(google::protobuf::RpcController *controller,
                           const ::tew_im::PutSingleFileReq *request,
                           ::tew_im::PutSingleFileRsp *response,
                           ::google::protobuf::Closure *done)
        {
            brpc::ClosureGuard rpc_guard(done);
            response->set_request_id(request->request_id());
            // 1.首先给文件生成一个唯一的uuid作为文件的ID
            std::string fid = Uuid();
            // 2.获取请求中的文件数据，写入到磁盘中
            std::string body = request->file_data().file_content();
            bool ret = WriteFile(fid, body);
            if (ret == false)
            {
                response->set_success(false);
                response->set_errmsg("打开文件写入数据失败");
                ERROR_LOG("{}文件，写入失败", fid);
                return;
            }
            // 3.组织响应(获取可以直接通过对象获取，设置就需要获取指针对象设置)
            response->set_success(true);
            response->mutable_file_info()->set_file_id(fid);
            response->mutable_file_info()->set_file_name(request->file_data().file_name());
            response->mutable_file_info()->set_file_size(request->file_data().file_size());
        }
        // 多文件上传
        void PutMultiFile(google::protobuf::RpcController *controller,
                          const ::tew_im::PutMultiFileReq *request,
                          ::tew_im::PutMultiFileRsp *response,
                          ::google::protobuf::Closure *done)
        {
            brpc::ClosureGuard rpc_guard(done);
            response->set_request_id(request->request_id());
            for (int i = 0; i < request->file_data_size(); i++)
            {
                // 1.首先给文件生成一个唯一的uuid作为文件的ID
                std::string fid = Uuid();
                // 2.获取请求中的文件数据，写入到磁盘中
                std::string body = request->file_data(i).file_content();
                bool ret = WriteFile(fid, body);
                if(ret==false)
                {
                    response->set_success(false);
                    response->set_errmsg("打开文件写入数据失败");
                    ERROR_LOG("{}文件，写入失败", fid);
                    return;
                }
                // 3.组织响应
                FileMessageInfo *info = response->add_file_info(); // 在rsp生成一个对象，然后返回该对象指针，让外部设置进去
                info->set_file_id(fid);
                info->set_file_name(request->file_data(i).file_name());
                info->set_file_size(request->file_data(i).file_size());
            }
            response->set_success(true);
        }
        // 下载单个文件
        void GetSingleFile(google::protobuf::RpcController *controller,
                           const ::tew_im::GetSingleFileReq *request,
                           ::tew_im::GetSingleFileRsp *response,
                           ::google::protobuf::Closure *done)
        {
            brpc::ClosureGuard rpc_guard(done);
            response->set_request_id(request->request_id());
            // 1.获取请求中的文件id
            std::string fid = request->file_id();
            // 2.根据文件id打开文件，读取数据
            std::string body;
            bool ret = ReadFile(fid, body);
            if (ret == false)
            {
                response->set_success(false);
                response->set_errmsg("打开文件，读取失败");
                ERROR_LOG("{}文件，读取失败", fid);
                return;
            }
            // 3.组织响应
            response->set_success(true);
            response->mutable_file_data()->set_file_id(fid);
            response->mutable_file_data()->set_file_content(body);
        }
        // 下载多个文件
        void GetMultiFile(google::protobuf::RpcController *controller,
                          const ::tew_im::GetMultiFileReq *request,
                          ::tew_im::GetMultiFileRsp *response,
                          ::google::protobuf::Closure *done)
        {
            brpc::ClosureGuard rpc_guard(done);
            response->set_request_id(request->request_id());
            for (int i = 0; i < request->file_id_list_size(); i++)
            {
                // 1.获取请求中的文件id
                std::string fid = request->file_id_list(i);
                // 2.根据文件id打开文件，读取数据
                std::string body;
                bool ret = ReadFile(fid, body);
                if (ret == false)
                {
                    response->set_success(false);
                    response->set_errmsg("打开文件，读取失败");
                    ERROR_LOG("{}文件，读取失败", fid);
                    return;
                }
                //组织响应
                FileDownloadData data;
                data.set_file_id(fid);
                data.set_file_content(body);
                response->mutable_file_data()->insert({fid,data});
            }
            response->set_success(true);
        }
    private:
    };
    /*因为brpc+etcd服务器的创建和配置代码还是比较多的,并不想放入main函数中，所以再封装一个FileServer类,将创建过程全部封装起来
    只向外提供启动服务器接口*/
    class FileServer
    {
    public:
        using ptr = std::shared_ptr<FileServer>;
        // 太多参数了，太乱了，通过接口方式来先构建子模块，再构建FileServer对象
        FileServer(const Registry::ptr &reg_client,
                   const std::shared_ptr<brpc::Server> &rpc_server)
            : _reg_client(reg_client), _rpc_server(rpc_server) {}
        // 文件存储子服务主要由三个模块组成：注册中心客户端+文件读写模块+brpc服务器
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }

    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
    // 通过建造者模式，来封装构建过程，便于扩展
    class FileScrBuild
    {
    public:
        // 用于构建注册中心客户端对象，并进行服务注册
        void make_reg(const std::string &reg_host, const std::string &service_name, const std::string &svr_host)
        {
            _reg_client = std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name, svr_host);
        }
        // 用于构建rpc服务器，并启动服务器(start)
        void make_rpc(uint16_t listen_port, int32_t idle_timeout, uint8_t num_thread)
        {
            // 创建一个rpc服务器
            _rpc_server = std::make_shared<brpc::Server>();
            // 3.向rpc服务器对象中新增serverImpl服务,以及对应的配置选项
            FileSeriveImpl *file_service = new FileSeriveImpl(); // 必须在堆上创建该对象，不然函数结束该服务对象就会被销毁，等rpc调用时就会出错
            int ret = _rpc_server->AddService(file_service, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            if (ret == -1)
            {
                ERROR_LOG("添加rpc服务失败");
                return; // 一定要用abort吗？
            }
            // 4.启动服务器,设置对应的端口及连接配置选项
            brpc::ServerOptions opt;
            opt.idle_timeout_sec = idle_timeout; // 连接超时后断开，-1表示关闭这个功能
            opt.num_threads = num_thread;        // 服务器中进行io线程的数量
            ret = _rpc_server->Start(listen_port, &opt);
            if (ret == -1)
            {
                ERROR_LOG("启动rpc服务器失败");
                return;
            }
        }
        // 返回构造完毕的SpeechSever服务器对象
        FileServer::ptr build()
        {
            // 可以进行一些检测，检测子对象都是否创建好
            if (!_rpc_server)
            {
                ERROR_LOG("初始化rpc服务器模块失败");
                abort();
            }
            if (!_reg_client)
            {
                ERROR_LOG("初始化服务注册中心模块失败");
                abort();
            }
            // 都没有问题了，就可以返回创建的SpeechServer对象了
            return std::make_shared<FileServer>(_reg_client, _rpc_server);
        }

    private:
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
}
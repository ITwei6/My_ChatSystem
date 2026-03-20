/*实现语音识别子服务逻辑主要分为三部分
1.先实现语音识别的服务类，也就是重写实现具体的业务处理
2.封装实现一个语音识别子服务的服务器类(本质底层是rpc服务器)
3.实现语音识别子服务类的建造者*/
#include <brpc/server.h>
#include <butil/logging.h>
#include "speech.pb.h" //protobuf生成的rpc框架代码
#include "logger.hpp"  //日志模块封装
#include "etcd.hpp"    //注册中心模块封装
#include "asr.hpp"     //语音识别模块封装，不需要指定路径，最后通过cmake指定头文件路径，就可以找到
namespace tew_im
{
    /*语音识别子服务业务逻辑：
    1.接收请求，获取请求中的语音数据speech_content  2.调用ASRClient接口，获取结果 3.组织响应发送回去
    */
    class SpeechSeriveImpl : public tew_im::SpeechService
    {
    public:
        SpeechSeriveImpl(const ASRClient::ptr &asr_client)
            : _asr_client(asr_client) {}
        ~SpeechSeriveImpl() {}
        void SpeechRecognition(google::protobuf::RpcController *controller,
                               const ::tew_im::SpeechRecognitionReq *request,
                               ::tew_im::SpeechRecognitionRsp *response,
                               ::google::protobuf::Closure *done)
        {
            brpc::ClosureGuard rpc_guard(done);
            auto rsp = _asr_client->recongize(request->speech_content());
            if (rsp.empty())
            {
                ERROR_LOG("{}:语音识别出错", request->request_id());
                // 构建一个响应返回
                response->set_request_id(request->request_id());
                response->set_success(false);
                response->set_errmsg("语音识别出错");
                return;
            }
            // 识别成功了,组织正确的响应发送回去
            response->set_request_id(request->request_id());
            response->set_success(true);
            response->set_recognition_result(rsp);
        }

    private:
        ASRClient::ptr _asr_client;
    };
    /*因为brpc+etcd服务器的创建和配置代码还是比较多的,并不想放入main函数中，所以再封装一个SpeechServer类,将创建过程全部封装起来
    只向外提供启动服务器接口*/
    class SpeechServer
    {
    public:
        using ptr = std::shared_ptr<SpeechServer>;
        // 太多参数了，太乱了，通过接口方式来先构建子模块，再构建SpeechSever对象
        SpeechServer(const ASRClient::ptr &asr_client,
                     const Registry::ptr &reg_client,
                     const std::shared_ptr<brpc::Server> &rpc_server)
            : _asr_client(asr_client), _reg_client(reg_client), _rpc_server(rpc_server) {}
        // 语音识别子服务主要由三个模块组成：注册中心客户端+语音识别客户端+brpc服务器
        void start()
        {
            _rpc_server->RunUntilAskedToQuit();
        }

    private:
        ASRClient::ptr _asr_client;
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
    // 通过建造者模式，来封装构建过程，便于扩展
    class SpeechScrBuild
    {
    public:
        // 用于构建语音识别客户端对象
        void make_asr(const std::string &app_id, const std::string &api_key, const std::string &secret_key)
        {
            _asr_client = std::make_shared<ASRClient>(app_id, api_key, secret_key);
        }
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
            SpeechSeriveImpl *speech_service = new SpeechSeriveImpl(_asr_client); // 必须在堆上创建该对象，不然函数结束该服务对象就会被销毁，等rpc调用时就会出错
            int ret = _rpc_server->AddService(speech_service, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
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
        SpeechServer::ptr build()
        {
            // 可以进行一些检测，检测子对象都是否创建好
            if (!_asr_client)
            {
                ERROR_LOG("初始化语音识别模块失败");
                abort();
            }
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
            return std::make_shared<SpeechServer>(_asr_client, _reg_client, _rpc_server);
        }

    private:
        ASRClient::ptr _asr_client;
        Registry::ptr _reg_client;
        std::shared_ptr<brpc::Server> _rpc_server;
    };
}

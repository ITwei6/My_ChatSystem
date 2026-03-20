/*实现语音识别子服务
1.进行参数解析
2.日志初始化
3.构建语音识别服务器
4.启动服务器*/

#include <gflags/gflags.h>
#include <thread>
#include "speech_server.hpp"
DEFINE_bool(run_mode, false, "表示程序的运行模式，默认是false调试默认，true表示发布模式");
DEFINE_string(file, "", "表示发布模式下要输出的文件名称，默认调试模式下为空");
DEFINE_int32(level, 0, "表示发布模式下日志器输出的等级，默认调试模式下为0");

/*语音识别子模块需要的参数解析*/
DEFINE_string(app_id, "120960306", "语音识别平台id");
DEFINE_string(api_key, "1FqlQPClhShFvaGrZONvqeGX", "语音识别平台密钥");
DEFINE_string(sercert_key, "FlIkGLFFdyX8vH4s7mAcN3EhoCkoxWyM", "语音识别平台保密密钥");
/*注册中心客户端模块需要的参数解析*/
DEFINE_string(reg_host, "http://127.0.0.1:2379", "服务注册中心的地址");
DEFINE_string(basedir, "/service", "要注册服务的目录，即发现端要监控的目录");
DEFINE_string(instance_name, "/speech/instance", "注册的服务实例名称");
DEFINE_string(host_id, "127.0.0.1:10004", "提供该服务对应的主机地址");
/*rpc服务器模块需要的参数解析*/
DEFINE_int32(listend_host_id, 10004, "rpc服务器监听的端口");
DEFINE_int32(time_out, -1, "连接超时时间，-1表示关闭");
DEFINE_int32(num_thread, 1, "rpc服务器线程池数量");

/*建造者模式 (SpeechScrBuild)：非常适合这种需要复杂配置才能构建的对象。它将对象的构建过程与表示分离，
使得同样的构建过程可以创建不同的表示，并且大大简化了 main 函数的逻辑。*/
int main(int argc, char *argv[])
{
    // 首先初始化gflags框架，告诉它要捕捉命令行中的参数数据
    google::ParseCommandLineFlags(&argc, &argv, true);
    // 初始化spdlog日志器；
    tew_im::init_logger(FLAGS_run_mode, FLAGS_file, FLAGS_level);
    // 关闭服务端的输出日志
    logging::LoggingSettings settings;
    settings.logging_dest = logging::LoggingDestination::LOG_TO_NONE;
    logging::InitLogging(settings);

    /*创建一个SpeechServer对象，然后启动服务器即可*/
    tew_im::SpeechScrBuild ssb;
    ssb.make_asr(FLAGS_app_id, FLAGS_api_key, FLAGS_sercert_key);
    ssb.make_rpc(FLAGS_listend_host_id, FLAGS_time_out, FLAGS_num_thread);
    ssb.make_reg(FLAGS_reg_host, FLAGS_basedir + FLAGS_instance_name, FLAGS_host_id);
    // 注册的服务是：/service/speech/instance ：127.0.0.1:8888
    tew_im::SpeechServer::ptr svr = ssb.build();
    svr->start();
}

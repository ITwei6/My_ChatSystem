#include "../../common/rabbitmq.hpp"
#include <gflags/gflags.h>
DEFINE_bool(run_mode,false,"表示程序的运行模式，默认是false调试默认，true表示发布模式");
DEFINE_string(file,"","表示发布模式下要输出的文件名称，默认调试模式下为空");
DEFINE_int32(level,0,"表示发布模式下日志器输出的等级，默认调试模式下为0");

DEFINE_string(user, "root", "rabbitmq的用户名");
DEFINE_string(passwd, "123456", "rabbitmq服务器的密码");
DEFINE_string(host, "127.0.0.1:5672/", "rabbitmq服务器主机地址");

void callback(const char*body,size_t len)
{
    std::string msg;
    msg.assign(body,len);
    std::cout<<msg<<std::endl;
}
int main(int argc,char*argv[])
{
    //首先初始化gflags框架，告诉它要捕捉命令行中的参数数据
    google::ParseCommandLineFlags(&argc, &argv, true);
    //初始化spdlog日志器；
    init_logger(FLAGS_run_mode,FLAGS_file,FLAGS_level); 

    MQClient client(FLAGS_user,FLAGS_passwd,FLAGS_host);
    client.delcarcomponent("test-exchange","test-queue");
    client.consume("test-queue",callback);
    std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}
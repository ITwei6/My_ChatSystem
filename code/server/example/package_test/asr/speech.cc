#include "../../common/asr.hpp"
#include <gflags/gflags.h>
DEFINE_bool(run_mode,false,"表示程序的运行模式，默认是false调试默认，true表示发布模式");
DEFINE_string(file,"","表示发布模式下要输出的文件名称，默认调试模式下为空");
DEFINE_int32(level,0,"表示发布模式下日志器输出的等级，默认调试模式下为0");

DEFINE_string(app_id, "120960306", "语音识别平台id");
DEFINE_string(api_key, "1FqlQPClhShFvaGrZONvqeGX", "语音识别平台密钥");
DEFINE_string(sercert_key, "FlIkGLFFdyX8vH4s7mAcN3EhoCkoxWyM", "语音识别平台保密密钥");

int main(int argc,char*argv[])
{
    //首先初始化gflags框架，告诉它要捕捉命令行中的参数数据
    google::ParseCommandLineFlags(&argc, &argv, true);
    //初始化spdlog日志器；
    init_logger(FLAGS_run_mode,FLAGS_file,FLAGS_level);
    ASRClient client(FLAGS_app_id,FLAGS_api_key,FLAGS_sercert_key);

    std::string file_content;
    aip::get_file_content("16k.pcm", &file_content);
    std::string res=client.recongize(file_content);
    if(res.empty())
    {
        std::cout<<"识别错误"<<std::endl;
        return -1;
    }
    std::cout<<res<<std::endl;
}
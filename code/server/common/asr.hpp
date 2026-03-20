#pragma
#include "aip-cpp-sdk/speech.h"
#include "logger.hpp"
namespace tew_im
{
    class ASRClient
    {
    public:
        using ptr = std::shared_ptr<ASRClient>;
        ASRClient(const std::string &app_id, const std::string &api_key, const std::string &secret_key)
            : _client(app_id, api_key, secret_key) {}
        // 给我一个语音数据，我给你返回对应的识别结果
        std::string recongize(const std::string &speech_data)
        {
            Json::Value result = _client.recognize(speech_data, "pcm", 16000, aip::null);
            if (result["err_no"].asInt() != 0)
            {
                ERROR_LOG("识别错误{}", result["err_msg"].asCString())
                return std::string();
            }
            return result["result"][0].asString();
        }

    private:
        aip::Speech _client;
    };
}
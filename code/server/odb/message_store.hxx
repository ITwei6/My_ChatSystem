#pragma once
#include <string>
#include <cstddef>
#include <odb/nullable.hxx>
#include <odb/core.hxx>
#include <memory>
#include <boost/date_time/posix_time/posix_time.hpp>
namespace tew_im
{
#pragma db object table("message_store")
    // 映射聊天会话成员表
    class MessageStore // 类就是一个聊天会话成员表
    {
    public:
        MessageStore() {}
        MessageStore(const std::string &msg_id,
                     const std::string &sender_id,
                     const std::string &csid,
                     boost::posix_time::ptime ctime,
                     unsigned char mtype)
            : _message_id(msg_id), _sender_id(sender_id), _chat_session_id(csid), _create_time(ctime), _type(mtype)
        {
        } // 这些参数都是必须要存在的，其他的可能会不存在，就通过接口来进行初始化
        void message_id(const std::string &value) { _message_id = value; }
        std::string message_id() { return _message_id; }

        void sender_id(const std::string &value) { _sender_id = value; }
        std::string sender_id() { return _sender_id; }

        void chat_session_id(const std::string &value) { _chat_session_id = value; }
        std::string chat_session_id() { return _chat_session_id; }

        void create_time(const boost::posix_time::ptime &value) { _create_time = value; }
        boost::posix_time::ptime create_time() { return _create_time; }

        void type(const unsigned char &value) { _type = value; }
        unsigned char type() { return _type; }

        void msg_content(const std::string &value) { _msg_content = value; }
        std::string msg_content()
        {
            if (!_msg_content)
                return std::string();
            return *_msg_content;
        }

        void file_id(const std::string &value) { _file_id = value; }
        std::string file_id()
        {
            if (!_file_id)
                return std::string();
            return *_file_id;
        }

        void file_size(const int &value) { _file_size = value; }
        int file_size()
        {
            if (!_file_size)
                return int();
            return *_file_size;
        }
        void file_name(const std::string &value) { _file_name = value; }
        std::string file_name()
        {
            if (!_file_name)
                return int();
            return *_file_name;
        }

    private:
        friend class odb::access;
#pragma db id auto
        unsigned long _id;
#pragma db type("varchar(64)") index unique
        std::string _message_id; // 消息的唯一id
#pragma db type("varchar(64)")
        std::string _sender_id;       // 发送者的id
#pragma db type("varchar(64)") index  // 通常需要作为筛选条件进行查询，会出现重复
        std::string _chat_session_id; // 所属的聊天会话id
#pragma db type("TIMESTAMP")
        boost::posix_time::ptime _create_time;   // 消息的产生时间
        unsigned char _type;                     // 消息的类型
        odb::nullable<std::string> _msg_content; // 消息的内容，只存储文本消息，文件/语言/图像不存储，都存储在文件子服务中

// 下面三个是针对文件类型的数据的，如果不是文件类型的数据，就用不到
#pragma db type("varchar(64)")
        odb::nullable<std::string> _file_id; // 针对文件/图片/语音等数据会有
#pragma db type("varchar(128)")
        odb::nullable<std::string> _file_name;
        odb::nullable<int> _file_size;
    };
}
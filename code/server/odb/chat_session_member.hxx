#pragma once
#include <string>
#include <cstddef>
#include <odb/nullable.hxx>
#include <odb/core.hxx>
#include <memory>
namespace tew_im
{
#pragma db object table("chat_session_member")
    // 映射聊天会话成员表
    class ChatSessionMember // 类就是一个聊天会话成员表
    {
    public:
        ChatSessionMember() {}
        ChatSessionMember(const std::string &csid, const std::string &uid)
            : _chat_session_id(csid), _user_id(uid) {} // 对象就是一个聊天会话

        std::string chat_session_id() { return _chat_session_id; }
        void chat_session_id(const std::string &sid) { _chat_session_id = sid; }

        std::string user_id() { return _user_id; }
        void user_id(const std::string &uid) { _user_id = uid; }

    private:
        friend class odb::access;
#pragma db id auto
        unsigned long _id;
#pragma db type("varchar(64)") index  // 因为一个聊天会话中会有多个用户存在，所有聊天会话会存在重复
        std::string _chat_session_id; // 并且通常用聊天会话id来进行查询
#pragma db type("varchar(64)")
        std::string _user_id;
    };
}
// odb -d mysql --std c++11 --generate-query --generate-schema --profile boost/date-time person.hxx

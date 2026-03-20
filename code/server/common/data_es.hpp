#pragma once
#include "es.hpp"
#include "user.hxx"
#include "message_store.hxx"
#include "logger.hpp"
namespace tew_im
{
    class ESClientFactory
    {
    public:
        static std::shared_ptr<elasticlient::Client> create(const std::vector<std::string> host_list)
        {
            return std::make_shared<elasticlient::Client>(host_list);
        }
    };
    class ESUser
    {
    public:
        using ptr = std::shared_ptr<ESUser>;
        ESUser(const std::shared_ptr<elasticlient::Client> &client) : _es_client(client) {}
        bool createIndex()
        {
            bool ret = ESIndex(_es_client, "user")
                           .append("user_id", "keyword", "standard", true)
                           .append("nickname")
                           .append("phone", "keyword", "standard", true)
                           .append("description", "text", "standard", false)
                           .append("avatar_id", "keyword", "standard", false)
                           .create();
            if (ret == false)
            {
                INFO_LOG("用户信息索引创建失败!");
                return false;
            }
            INFO_LOG("用户信息索引创建成功!");

            return true;
        }
        bool appendData(const std::string &uid,
                        const std::string &phone,
                        const std::string &nickname,
                        const std::string &description,
                        const std::string &avatar_id)
        {
            bool ret = ESInsert(_es_client, "user")
                           .append("user_id", uid)
                           .append("nickname", nickname)
                           .append("phone", phone)
                           .append("description", description)
                           .append("avatar_id", avatar_id)
                           .insert(uid); // 当前消息的唯一索引id,后续 都是可以通过这个id找到该消息,然后删除
            if (ret == false)
            {
                ERROR_LOG("用户数据插入/更新失败!");

                return false;
            }
            INFO_LOG("用户数据新增/更新成功!");
            return true;
        }
        std::vector<User> search(const std::string &key, const std::vector<std::string> &uid_list)
        {
            std::vector<User> res;
            Json::Value json_user = ESSearch(_es_client, "user")
                                        .append_should_match("phone.keyword", key)
                                        .append_should_match("user_id.keyword", key)
                                        .append_should_match("nickname", key)
                                        .append_must_not_terms("user_id.keyword", uid_list)
                                        .search();
            if (json_user.isArray() == false)
            {
                ERROR_LOG("用户搜索结果为空，或者结果不是数组类型");
                return res;
            }
            int sz = json_user.size();
            DEBUG_LOG("检索结果条目数量：{}", sz);

            for (int i = 0; i < sz; i++)
            {
                User user;
                user.user_id(json_user[i]["_source"]["user_id"].asString());
                user.nickname(json_user[i]["_source"]["nickname"].asString());
                user.description(json_user[i]["_source"]["description"].asString());
                user.phone(json_user[i]["_source"]["phone"].asString());
                user.avatar_id(json_user[i]["_source"]["avatar_id"].asString());
                res.push_back(user);
            }
            return res;
        }

    private:
        // const std::string _uid_key = "user_id";
        // const std::string _desc_key = "user_id";
        // const std::string _phone_key = "user_id";
        // const std::string _name_key = "user_id";
        // const std::string _avatar_key = "user_id";
        std::shared_ptr<elasticlient::Client> _es_client;
    };
    class ESMessage
    {
    public:
        using ptr = std::shared_ptr<ESMessage>;
        ESMessage(const std::shared_ptr<elasticlient::Client> &client) : _es_client(client) {}
        // 1.创建es消息索引
        bool createIndex()
        {
            bool ret = ESIndex(_es_client, "message")
                           .append(_chat_session_id, "keyword", "standard", true)
                           .append(_message_id, "keyword", "standard", false)
                           .append(_sender_id, "keyword", "standard", false)
                           .append(_create_time, "long", "standard", false)
                           .append(_msg_content)
                           .create();
            if (ret == false)
            {
                INFO_LOG("消息存储索引创建失败!");
                return false;
            }
            INFO_LOG("消息存储索引创建成功!");

            return true;
        }
        // 往索引中新增数据
        bool appendData(const std::string &chat_session_id,
                        const std::string &message_id,
                        const long &create_time,
                        const std::string &sender_id,
                        const std::string &msg_content)
        {
            bool ret = ESInsert(_es_client, "message")
                           .append(_chat_session_id, chat_session_id)
                           .append(_message_id, message_id)
                           .append(_create_time, create_time)
                           .append(_sender_id, sender_id)
                           .append(_msg_content, msg_content)
                           .insert(message_id); // 当前消息的唯一索引id,后续 都是可以通过这个id找到该消息,然后删除
            if (ret == false)
            {
                ERROR_LOG("消息数据插入/更新失败!");

                return false;
            }
            INFO_LOG("消息数据新增/更新成功!");
            return true;
        }
        std::vector<MessageStore> search(const std::string &key_msg, const std::string &chat_sesssion_id)
        {
            std::vector<MessageStore> res;
            Json::Value json_user = ESSearch(_es_client, "message")
                                        .append_must_terms(_chat_session_id + ".keyword", chat_sesssion_id)
                                        .append_must_match(_msg_content, key_msg)
                                        .search();
            if (json_user.isArray() == false)
            {
                ERROR_LOG("用户搜索结果为空，或者结果不是数组类型");
                return res;
            }
            int sz = json_user.size();
            DEBUG_LOG("检索结果条目数量：{}", sz);

            for (int i = 0; i < sz; i++)
            {
                MessageStore msg;
                msg.chat_session_id(json_user[i]["_source"][_chat_session_id].asString());
                msg.message_id(json_user[i]["_source"][_message_id].asString());

                // json_user[i]["_source"][_create_time].asInt64()获取到的是时间戳,但是message中要的是ptime
                boost::posix_time::ptime ctime = boost::posix_time::from_time_t(json_user[i]["_source"][_create_time].asInt64());
                msg.create_time(ctime);
                msg.sender_id(json_user[i]["_source"][_sender_id].asString());
                msg.msg_content(json_user[i]["_source"][_msg_content].asString());
                res.push_back(msg);
            }
            return res;
        }

    private:
        const std::string _chat_session_id = "chat_session_id";
        const std::string _message_id = "message_id";
        const std::string _create_time = "create_time";
        const std::string _sender_id = "sender_id";
        const std::string _msg_content = "msg_content";
        std::shared_ptr<elasticlient::Client> _es_client;
    };
}
#pragma once
#include <string>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <odb/database.hxx>
#include <odb/mysql/database.hxx>
#include "message_store.hxx"
#include "message_store-odb.hxx"
#include "logger.hpp"
#include <vector>
#include "mysql.hpp"

namespace tew_im
{
    class MessageStoreTable
    {
    public:
        using ptr = std::shared_ptr<MessageStoreTable>;
        MessageStoreTable(const std::shared_ptr<odb::mysql::database> &db) : _db(db) {}
        // 主要对消息存储表进行四个操作：
        // 1.新增一条消息(消息id，发送者id，所属会话id，创建时间，消息类型...)
        bool append(MessageStore &msg)
        {
            try
            {
                // 3.获取事务对象，开启事务
                odb::transaction trans(_db->begin());
                // 4.数据库相关操作，增删查改
                _db->persist(msg);
                // 5.提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("新增一条消息记录出错：{}-{}-{}", msg.message_id(), msg.chat_session_id(), e.what());
                return false;
            }
            return true;
        }
        // 2.移除指定聊天会话的消息---用于删除好友时，将与好友的聊天记录删除
        bool remove(const std::string &csid)
        {
            // delete from table where chat_session_id=csid and user_id=uid
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<MessageStore> query; // 给类型去别名，原类型太长了
                _db->erase_query<MessageStore>(query::chat_session_id == csid);
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("删除指定聊天会话中的消息：{}-{}", csid, e.what());
                return false;
            }
            return true;
        }
        // 3.获取指定聊天会话的最近n条消息--用于点击好友，显示与好友最近的n条消息
        std::vector<MessageStore> recent(const std::string &csid, int n)
        {
            // select *from messagestore where chat_session='csid' ordery by create_time desc limit n
            std::vector<MessageStore> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<MessageStore> query;
                typedef odb::result<MessageStore> result;
                std::stringstream condtion;
                condtion << "chat_session_id='" << csid << "'";
                condtion << "order by create_time desc limit " << n;
                result r(_db->query<MessageStore>(condtion.str()));
                for (auto it = r.begin(); it != r.end(); it++)
                {
                    res.push_back(*it);
                }
                trans.commit();
                return res;
            }
            catch (std::exception &e)
            {
                ERROR_LOG("获取指定会话最近n条消息失败：{}-{}", csid, e.what());
                return {};
            }
        }
        // 4.获取指定聊天会话的时间范围内的消息--用来按照时间范围来查询消息
        std::vector<MessageStore> range(const std::string &csid,
                                        const boost::posix_time::ptime &stime, const boost::posix_time::ptime &etime)
        {
            std::vector<MessageStore> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<MessageStore> query;
                typedef odb::result<MessageStore> result;
                result r(_db->query<MessageStore>(query::chat_session_id == csid &&
                                                  query::create_time >= stime && query::create_time <= etime));
                for (auto it = r.begin(); it != r.end(); it++)
                {
                    res.push_back(*it);
                }
                trans.commit();
                return res;
            }
            catch (std::exception &e)
            {
                ERROR_LOG("获取指定会话在{}-{}范围的消息：{}-{}", boost::posix_time::to_simple_string(stime), boost::posix_time::to_simple_string(etime), csid, e.what());
                return {};
            }
        }

    private:
        std::shared_ptr<odb::mysql::database> _db;
    };
}
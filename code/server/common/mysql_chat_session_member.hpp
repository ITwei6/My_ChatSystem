#pragma once
#include <string>
#include <memory>
#include <cstdlib>
#include <iostream>
#include "chat_session_member.hxx"
#include "chat_session_member-odb.hxx"
#include "logger.hpp"
#include <vector>
#include "mysql.hpp"
namespace tew_im
{
    class ChatSessionMemberTable
    {
    public:
        using ptr = std::shared_ptr<ChatSessionMemberTable>;
        // • 向指定会话中添加单个成员
        ChatSessionMemberTable(const std::shared_ptr<odb::mysql::database> &db) : _db(db) {}
        bool append(ChatSessionMember &csm)
        {
            try
            {
                // 3.获取事务对象，开启事务
                odb::transaction trans(_db->begin());
                // 4.数据库相关操作，增删查改
                _db->persist(csm);
                // 5.提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("新增一条聊天会话成员记录出错：{}-{}-{}", csm.chat_session_id(), csm.user_id(), e.what());
                return false;
            }
            return true;
        }
        // • 向指定会话中添加多个成员。
        bool append(std::vector<ChatSessionMember> &csms)
        {
            try
            {
                odb::transaction trans(_db->begin());
                for (auto &csm : csms)
                    _db->persist(csm);
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("新增多条聊天会话成员记录出错：{}-{}", csms[0].chat_session_id(), e.what());
                return false;
            }
            return true;
        }
        // • 从指定会话中删除单个成员
        bool remove(const std::string &csid, const std::string &uid)
        {
            // delete from table where chat_session_id=csid and user_id=uid
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<ChatSessionMember> query; // 给类型去别名，原类型太长了
                _db->erase_query<ChatSessionMember>(query::chat_session_id == csid && query::user_id == uid);
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("从指定会话中删除单个成员出错：{}-{}-{}", csid, uid, e.what());
                return false;
            }
            return true;
        }
        // • 通过会话 ID，获取会话的所有成员 ID
        std::vector<std::string> members_id(const std::string &csid)
        {
            std::vector<std::string> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<ChatSessionMember> query;
                typedef odb::result<ChatSessionMember> result;
                result r(_db->query<ChatSessionMember>(query::chat_session_id == csid));
                for (auto it = r.begin(); it != r.end(); it++)
                {
                    res.push_back(it->user_id());
                }
                trans.commit();
                return res;
            }
            catch (std::exception &e)
            {
                ERROR_LOG("通过会话 ID，获取会话的所有成员 ID出错：{}-{}", csid, e.what());
                return {};
            }
        }
        // • 删除会话所有成员：在删除会话的时候使用
        bool remove_all(const std::string &csid)
        {
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<ChatSessionMember> query; // 给类型去别名，原类型太长了
                _db->erase_query<ChatSessionMember>(query::chat_session_id == csid);
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("删除会话所有成员：{}-{}", csid, e.what());
                return false;
            }
            return true;
        }

    private:
        std::shared_ptr<odb::mysql::database> _db;
    };
}
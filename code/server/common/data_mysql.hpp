
#pragma once
#include <string>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <odb/database.hxx>
#include <odb/mysql/database.hxx>
#include "user.hxx"
#include "user-odb.hxx"
#include <gflags/gflags.h>
#include "logger.hpp"
#include "mysql.hpp"
/*针对用户表进行相关操作,是基于数据库对象db句柄来操作的*/
// 要对用户表进行什么操作呢？--->根据具体的接口决定：
// 1.用户/手机号注册---在用户表中新增用户操作
// 2.用户登录---要根据用户名来获取到该用户的信息--匹配密码
// 3.手机号登录---要根据手机号来获取到该用户的信息
// 4.获取用户的信息---根据用户id来获取单个用户信息，或者用多个用户id获取多个用户信息
// 5.信息修改---用户表数据更新

// 构建数据库对象db的过程还是比较复杂，需要的参数也比较多，不想放在构造函数中实例化，选择使用一个工厂来直接构造
// 线程池个数--数据库用户--数据密码--具体使用的数据名称--数据库服务器地址--端口号--字符集
namespace tew_im
{

    class UserTable
    {
    public:
        using ptr = std::shared_ptr<UserTable>;
        UserTable(const std::shared_ptr<odb::mysql::database> &db) : _db(db) {}
        bool insert(const std::shared_ptr<User> &usr)
        {
            try
            {
                // 3.获取事务对象，开启事务
                odb::transaction trans(_db->begin());
                // 4.数据库相关操作，增删查改
                _db->persist(*usr);
                // 5.提交事务
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("插入一条新用户记录出错：{}", e.what());
                return false;
            }
            return true;
        }
        std::shared_ptr<User> select_by_nickname(const std::string &name)
        {
            std::shared_ptr<User> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query; // 给类型去别名，原类型太长了
                res.reset(_db->query_one<User>(query::nickname == name));
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("根据用户名称获取用户数据出错{}:{}", name, e.what());
            }
            return res;
        }
        std::shared_ptr<User> select_by_phone(const std::string &phone)
        {
            std::shared_ptr<User> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                res.reset(_db->query_one<User>(query::phone == phone));
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("根据手机号获取用户数据出错{}:{}", phone, e.what());
            }
            return res;
        }
        std::shared_ptr<User> select_by_user_id(const std::string &user_id)
        {
            std::shared_ptr<User> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                res.reset(_db->query_one<User>(query::user_id == user_id));
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("根据用户id获取用户数据出错{}:{}", user_id, e.what());
            }
            return res;
        }
        std::vector<User> select_by_mutli_id(std::vector<std::string> ids)
        {
            if (ids.empty())
            {
                return std::vector<User>();
            }
            std::vector<User> res;
            try
            {
                odb::transaction trans(_db->begin());
                typedef odb::query<User> query;
                typedef odb::result<User> result;
                // 需要构建一条语句：select *from User where user_id in('id1','id2'....)
                // 所以筛选条件里构建一个 user_id in('id1','id2'....)
                std::stringstream ss;
                ss << "user_id in (";
                for (int i = 0; i < ids.size(); i++)
                {
                    ss << "'" << ids[i] << "',";
                    //'id1','id2',
                }
                std::string condition = ss.str();
                condition.pop_back();
                condition += ')';
                result r(_db->query<User>(condition));
                for (auto it = r.begin(); it != r.end(); it++)
                {
                    res.push_back(*it);
                }
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("根据多用户id获取多用户数据出错:{}", e.what());
            }
            return res;
        }
        // 更新记录之前必须要先查询到该记录
        void update(const std::shared_ptr<User> &usr)
        {
            try
            {
                odb::transaction trans(_db->begin());
                _db->update(*usr);
                trans.commit();
            }
            catch (std::exception &e)
            {
                ERROR_LOG("更新用户记录出错：{}", e.what());
            }
        }

    private:
        std::shared_ptr<odb::mysql::database> _db;
    };

}
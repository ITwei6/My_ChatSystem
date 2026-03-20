#pragma once
#include <memory>
#include <odb/database.hxx>
#include <odb/mysql/database.hxx>
namespace tew_im
{
    class DataBaseFactory
    {
    public:
        static std::shared_ptr<odb::mysql::database> create(
            const std::string &user,
            const std::string &paswd,
            const std::string &db_name,
            const std::string &host,
            const int &port,
            const std::string &cset, int max_pool)
        {
            /*ODB操作流程*/
            // 1.构建线程池对象(构造连接池工厂配置对象)
            std::unique_ptr<odb::mysql::connection_pool_factory> cpf(new odb::mysql::connection_pool_factory(max_pool, 0));
            // 2.构建数据库操作database对象
            auto db = std::make_shared<odb::mysql::database>(user, paswd, db_name, host, port, "", cset, 0, std::move(cpf));
            return db;
        }
    };
}
// 主要实现语音识别子服务的服务器的搭建
#include "transmit_server.hpp"

// 日志模块
DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");
// 服务注册中心模块
DEFINE_string(registry_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(instance_name, "/transmit_service/instance", "当前实例名称");
DEFINE_string(access_host, "127.0.0.1:10005", "当前实例的外部访问地址");
// Rpc服务器模块
DEFINE_int32(listen_port, 10005, "Rpc服务器监听端口");
DEFINE_int32(rpc_timeout, -1, "Rpc调用超时时间");
DEFINE_int32(rpc_threads, 1, "Rpc的IO线程数量");
// 服务发现模块
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(user_service, "/service/user_service", "用户管理子服务名称");

// mysql客户端模块
DEFINE_string(mysql_host, "127.0.0.1", "Mysql服务器访问地址");
DEFINE_string(mysql_user, "root", "Mysql服务器访问用户名");
DEFINE_string(mysql_pswd, "123456", "Mysql服务器访问密码");
DEFINE_string(mysql_db, "tew_im", "Mysql默认库名称");
DEFINE_string(mysql_cset, "utf8", "Mysql客户端字符集");
DEFINE_int32(mysql_port, 0, "Mysql服务器访问端口");
DEFINE_int32(mysql_pool_count, 4, "Mysql连接池最大连接数量");

// amqp://root:123456@127.0.0.1:5672/
DEFINE_string(mq_user, "root", "mq客户端用户名");
DEFINE_string(mq_passwd, "123456", "mq客户端密码");
DEFINE_string(mq_host, "127.0.0.1:5672", "mq服务端地址");
DEFINE_string(mq_exchange_name, "msg_exchage", "消息队列中发布交换机的名称");
DEFINE_string(mq_queue_name, "msg_queue", "消息队列中接收队列的名称");
DEFINE_string(mq_binding_key, "binding_key", "交换机与队列绑定的key");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    tew_im::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    tew_im::TransmitServerBuilder tsb;
    tsb.make_mq_object(FLAGS_mq_user, FLAGS_mq_passwd, FLAGS_mq_host, FLAGS_mq_exchange_name, FLAGS_mq_queue_name, FLAGS_mq_binding_key);
    tsb.make_mysql_object(FLAGS_mysql_user, FLAGS_mysql_pswd, FLAGS_mysql_host,
                          FLAGS_mysql_db, FLAGS_mysql_cset, FLAGS_mysql_port, FLAGS_mysql_pool_count);

    tsb.make_discovery_object(FLAGS_registry_host, FLAGS_base_service, FLAGS_user_service);
    tsb.make_rpc_server(FLAGS_listen_port, FLAGS_rpc_timeout, FLAGS_rpc_threads);
    tsb.make_registry_object(FLAGS_registry_host, FLAGS_base_service + FLAGS_instance_name, FLAGS_access_host);
    auto server = tsb.build();
    server->start();
    return 0;
}
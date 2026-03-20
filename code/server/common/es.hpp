#include <elasticlient/client.h>
#include <cpr/cpr.h>
#include <iostream>
#include "logger.hpp"
#include <json/json.h>
#include <iostream>
#include <sstream>
#include <memory>
bool Serialization(const Json::Value &val, std::string &dest)
{
    // 序列化接口StreamWriter的工厂类
    Json::StreamWriterBuilder swb;
    // 序列化接口StreamWriter
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
    // 通过序列化接口中的Write接口进行序列化
    std::stringstream ss;
    int ret = sw->write(val, &ss);
    if (ret != 0)
    {
        // std::cout << "json序列化失败\n";
        return false;
    }
    // std::cout << "json序列化成功\n";
    dest = ss.str();
    return true;
}

bool Deserialization(const std::string &str, Json::Value &val)
{
    // 反序列化接口CharReaderBuilder的工厂类
    Json::CharReaderBuilder crb;
    // 序列化接口CharReader
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
    // 通过序列化接口中的parse接口进行反序列化
    std::string err;
    bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), &val, &err);
    if (ret == false)
    {
        // std::cout << "json反序列化失败:" << err << std::endl;
        return false;
    }
    // std::cout << "json反序列化成功\n";
    return true;
}
// 利用es客户端，来访问es服务器，主要是构造其正文部分，正文部分是有指定的Value格式序列化而来
//  构建创建索引类,并确定索引里面中都有什么字段
class ESIndex
{
public:
    // 能够动态决定创建的索引名称，索引类型
    ESIndex(std::shared_ptr<elasticlient::Client> &client, const std::string &name, const std::string type = "_doc")
        : _client(client), _name(name), _type(type)
    {
        // 构建index的settings部分
        Json::Value analysis;
        Json::Value analyzer;
        Json::Value ik;
        Json::Value tokenizer;
        tokenizer["tokenizer"] = "ik_max_word";
        ik["ik"] = tokenizer;
        analyzer["analyzer"] = ik;
        analysis["analysis"] = analyzer;
        _index["settings"] = analysis;
    }
    // 指定该索引中都有哪些字段名称,该字段类型，该字段分词器类型，该字段是否启动索引等
    ESIndex &append(const std::string &key, const std::string &type = "text", const std::string analyzer = "ik_max_word", bool enabled = true)
    {
        // 首先构建properties中的Value对象，filed
        Json::Value fields;
        fields["type"] = type;
        fields["analyzer"] = analyzer;
        if (enabled == false)
            fields["enabled"] = enabled;
        _properties[key] = fields; // 该字段名称对应的各种属性字段
        return *this;
    }
    bool create(const std::string &index_id = "default_index_id")
    {
        Json::Value mappings;
        mappings["dynamic"] = true;
        mappings["properties"] = _properties;
        _index["mappings"] = mappings;

        // 指定格式的Json Value构建好，在使用es客户端发送请求时，先序列化
        std::string body;
        bool ret = Serialization(_index, body);
        if (ret == false)
        {
            ERROR_LOG("序列化索引错误\n");
            return false;
        }
        // INFO_LOG("创建索引的正文json格式，序列化完毕：{}",body);
        //  序列化成功之后，使用es客户端发送请求
        try
        {
            // 使用es客户端对es服务器进行数据查询
            auto rsp = _client->index(_name, _type, index_id, body);
            // 根据响应状态码判断是否成功
            if (rsp.status_code < 200 || rsp.status_code >= 300)
            {
                ERROR_LOG("创建es索引{}失败，响应状态码为：{}", _name, rsp.status_code);
                return false;
            }
        }
        catch (std::exception &e)
        {
            ERROR_LOG("捕捉到异常，创建索引{}失败：{}", _name, e.what());
            return false;
        }
        return true;
    }

private:
    std::string _name;                             // 索引名称
    std::string _type;                             // 索引类型
    Json::Value _index;                            // 用来构建直接Json格式的value
    Json::Value _properties;                       // 用来添加字段
    std::shared_ptr<elasticlient::Client> _client; // 用来访问es服务器的客户端
};

// 插入数据的封装，可以新增一个数据
class ESInsert
{
public:
    ESInsert(std::shared_ptr<elasticlient::Client> &client, const std::string &name, const std::string type = "_doc")
        : _client(client), _name(name), _type(type)
    {
    }
    // 指定插入对应字段的数据
    /*
    {
        "nickename":______
        "phone":______
    }
    */
    template <typename T>
    ESInsert &append(const std::string &key, const T &value)
    {
        _item[key] = value;
        return *this;
        // 可以连续插入索引中对应的字段
    }
    // 插入数据时要指定该组数据的id
    bool insert(const std::string &id)
    {
        // 指定格式的Json Value构建好，在使用es客户端发送请求时，先序列化
        std::string body;
        bool ret = Serialization(_item, body);
        if (ret == false)
        {
            ERROR_LOG("序列化新增数据Value错误\n");
            return false;
        }
        // INFO_LOG("创建索引的正文json格式，序列化完毕：{}",body);
        //  序列化成功之后，使用es客户端发送请求
        try
        {
            // 使用es客户端对es服务器进行数据插入
            auto rsp = _client->index(_name, _type, id, body);
            // 根据响应状态码判断是否成功
            if (rsp.status_code < 200 || rsp.status_code >= 300)
            {
                ERROR_LOG("插入数据{}失败，响应状态码为：{}", id, rsp.status_code);
                return false;
            }
        }
        catch (std::exception &e)
        {
            ERROR_LOG("捕捉到异常，插入数据{}失败：{}", id, e.what());
            return false;
        }
        return true;
    }

private:
    std::string _name;                             // 索引名称
    std::string _type;                             // 索引类型
    Json::Value _item;                             // 用来新增数据的Value格式
    std::shared_ptr<elasticlient::Client> _client; // 用来访问es服务器的客户端
};

// 移除数据封装，不需要构造正文,只需要指定删除的数据id
class ESremove
{
public:
    ESremove(std::shared_ptr<elasticlient::Client> &client, const std::string &name, const std::string type = "_doc")
        : _client(client), _name(name), _type(type)
    {
    }
    bool remove(const std::string &id)
    {
        //  序列化成功之后，使用es客户端发送请求
        try
        {
            // 使用es客户端对es服务器进行数据插入
            auto rsp = _client->remove(_name, _type, id);
            // 根据响应状态码判断是否成功
            if (rsp.status_code < 200 || rsp.status_code >= 300)
            {
                ERROR_LOG("删除数据{}失败，响应状态码为:{}", id, rsp.status_code);
                return false;
            }
        }
        catch (std::exception &e)
        {
            ERROR_LOG("捕捉到异常，删除数据{}失败：{}", id, e.what());
            return false;
        }
        return true;
    }

private:
    std::string _name;                             // 索引名称
    std::string _type;                             // 索引类型
    std::shared_ptr<elasticlient::Client> _client; // 用来访问es服务器的客户端
};

class ESSearch
{
public:
    ESSearch(std::shared_ptr<elasticlient::Client> &client, const std::string &name, const std::string type = "_doc")
        : _client(client), _name(name), _type(type)
    {
    }

    // 追加过滤条件，must_not_terms，要求是精准匹配
    ESSearch &append_must_not_terms(const std::string &key, const std::vector<std::string> &vals)
    {
        Json::Value fields;
        for (auto &val : vals)
        {
            fields[key].append(val);
        }
        Json::Value terms;
        terms["terms"] = fields;
        _must_not.append(terms);
        return *this;
    }
    // 追加过滤条件，should_match 要求是分词匹配
    ESSearch &append_should_match(const std::string &key, const std::string &val)
    {
        Json::Value field;
        field[key] = val;
        Json::Value match;
        match["match"] = field;
        // 向 ES 查询的 should 条件中添加一个 match 查询。
        _should.append(match);
        return *this;
    }
    // 追加过滤条件，must_terms，要求是精准匹配,必须都满足
    ESSearch &append_must_terms(const std::string &key, const std::string &val)
    {
        Json::Value field;
        field[key] = val;
        Json::Value term;
        term["term"] = field;
        // 向 ES 查询的 must 条件中添加一个 term 查询。
        _must.append(term);
        return *this;
    }
    // 追加过滤条件，must_match 要求是分词匹配
    ESSearch &append_must_match(const std::string &key, const std::string &val)
    {
        Json::Value field;
        field[key] = val;
        Json::Value match;
        match["match"] = field;
        // 向 ES 查询的 must 条件中添加一个 match 查询。
        _must.append(match);
        return *this;
    }
    Json::Value search()
    {
        Json::Value bools;
        if (_must_not.empty() == false)
            bools["must_not"] = _must_not;
        if (_should.empty() == false)
            bools["should"] = _should;
        if (_must.empty() == false)
            bools["must"] = _must;
        Json::Value query;
        query["bool"] = bools;
        Json::Value root;
        root["query"] = query;
        // 指定格式的Json Value构建好，在使用es客户端发送请求时，先序列化

        std::string body;
        bool ret = Serialization(root, body);
        if (ret == false)
        {
            ERROR_LOG("序列化索引错误\n");
            return Json::Value();
        }
        INFO_LOG("检索数据的正文：{}", body);
        //  序列化成功之后，使用es客户端发送请求
        cpr::Response rsp;
        try
        {
            // 使用es客户端对es服务器进行数据查询
            rsp = _client->search(_name, _type, body);
            // 根据响应状态码判断是否成功
            if (rsp.status_code < 200 || rsp.status_code >= 300)
            {
                ERROR_LOG("查询数据{}失败，响应状态码为:{}", body, rsp.status_code);
                return Json::Value();
            }
        }
        catch (std::exception &e)
        {
            ERROR_LOG("捕捉到异常，查询数{}失败：{}", body, e.what());
            return Json::Value();
        }

        INFO_LOG("检索数据响应的正文：[{}]", rsp.text);
        Json::Value json_res;
        ret = Deserialization(rsp.text, json_res);
        if (ret == false) // 因为最后的结果要Json对象，所以得到的响应正文还需要进行反序列化
        {
            ERROR_LOG("查询的响应结果正文反序列化失败\n");
            return Json::Value();
        }

        return json_res["hits"]["hits"];
    }

private:
    std::string _name;                             // 索引名称
    std::string _type;                             // 索引类型
    Json::Value _must_not;                         // 用来构造过滤条件的对象，必须不存在
    Json::Value _should;                           // 用来构造过滤条件的对象，存在一个即可
    Json::Value _must;                             // 用来构建过虑条件对象,必须都存在才可以
    std::shared_ptr<elasticlient::Client> _client; // 用来访问es服务器的客户端
};
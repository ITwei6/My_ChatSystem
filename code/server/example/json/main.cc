#include <json/json.h>
#include <iostream>
#include <sstream>
#include <memory>

bool Serialization(const Json::Value &val,std::string &dest)
{
    //序列化接口StreamWriter的工厂类
    Json::StreamWriterBuilder swb;
    //序列化接口StreamWriter
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
    //通过序列化接口中的Write接口进行序列化
    std::stringstream ss;
    int ret=sw->write(val,&ss);
    if(ret!=0)
    {
        std::cout<<"json序列化失败\n";
        return false;
    }
    std::cout<<"json序列化成功\n";
    dest=ss.str();
    return true;
}


bool Deserialization(const std::string &str,Json::Value &val)
{
    //反序列化接口CharReaderBuilder的工厂类
    Json::CharReaderBuilder crb;
    //序列化接口CharReader
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
    //通过序列化接口中的parse接口进行反序列化
    std::string err;
    bool ret=cr->parse(str.c_str(),str.c_str()+str.size(),&val,&err);
    if(ret==false)
    {
        std::cout<<"json反序列化失败:"<<err<<std::endl;
        return false;
    }
    std::cout<<"json反序列化成功\n";
    return true;
}
int main()
{
    char name[12]="tew";
    int age=22;
    float score[3]={90,99.9,100};
    //想对上面的数据进行序列化该怎么办？先用Json格式value对象存储，将对于的关系组织起来。
    //Json value 赋值
    Json::Value stu;
    stu["姓名"]=name;
    stu["年龄"]=age;
    stu["成绩"].append(score[0]);
    stu["成绩"].append(score[1]);
    stu["成绩"].append(score[2]);
    Json::Value fav;
    fav["阅读"]="西游记";
    fav["运动"]="打篮球";
    fav["听音乐"]="冬眠";
    stu["爱好"]=fav;
    //序列化
    std::string ret_string;
    Serialization(stu,ret_string);
    std::cout<<ret_string<<std::endl;
    //反序列化
    Json::Value val;
    Deserialization(ret_string,val);
    std::cout<<val["姓名"].asString()<<std::endl;
    std::cout<<val["年龄"].asInt()<<std::endl;
    int sz=val["成绩"].size();
    for(int i=0;i<sz;i++)
    {
        std::cout<<val["成绩"][i].asFloat()<<std::endl;
    }
    std::cout<<val["爱好"]["阅读"].asString()<<std::endl;
    std::cout<<val["爱好"]["运动"].asString()<<std::endl;
    std::cout<<val["爱好"]["听音乐"].asString()<<std::endl;
    
}
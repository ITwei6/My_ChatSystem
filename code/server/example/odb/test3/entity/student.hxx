#pragma once
#include <string>
#include <cstddef> // std::size_t
#include <boost/date_time/posix_time/posix_time.hpp>
#include <odb/nullable.hxx>
/*
 在 C++ 中，要使用 ODB 将类声明为持久化类，需要包含 ODB 的核心头文
件，并使用 #pragma db object 指令
 #pragma db object 指示 ODB 编译器将 person 类视为一个持久化类。
*/
#include <odb/core.hxx>
typedef boost::posix_time::ptime ptime;

#pragma db object // 将类与数据库表映射起来
class Student
{
    //类代表着一个表，一个类对象就代表表中一行记录
public: 
    Student(){};
    Student(unsigned long sn,std::string name,
        odb::nullable<unsigned short> age,unsigned long class_id):
        _sn(sn),_name(name),_age(age),_class_id(class_id){}
    void setsn(unsigned long sn){_sn=sn;}
    unsigned long getsn(){return _sn;}
    
    void setname(std::string name){_name=name;}
    std::string getname(){return _name;}
    
    void setage(unsigned long age){_age=age;}
    odb::nullable<unsigned short> getage(){return _age;}
    
    void setclassid(unsigned long classid){_class_id=classid;}
    unsigned long getclassid(){return _class_id;}
private:
    friend class odb::access;
    #pragma db id auto
    unsigned long _id;
    #pragma db unique
    unsigned long _sn;
    std::string _name;
    odb::nullable<unsigned short> _age;
    #pragma db index
    unsigned long _class_id;
};

#pragma db object
class Classes
{
public:
    //类代表着一个表，一个类对象就代表表中一行记录
    Classes(){}
    Classes(std::string name):_name(name){}

    void setname(std::string name){_name=name;}
    std::string getname(){return _name;}
private:
    friend class odb::access;
    #pragma db id auto
    unsigned long _id;
    std::string _name;
};

//视图通常用于复杂查询
//比如查询所有的学生信息，并显示班级名称
#pragma db view object(Student)\
                object(Classes = classes : Student::_class_id == classes::_id)\
                query((?))
class Student_Class
{
    public:
    #pragma db column(Student::_id)//指定类成员映射到数据库表中的列名
    unsigned long _id;
    #pragma db column(Student::_sn)//指定类成员映射到数据库表中的列名
    unsigned long _sn;
    #pragma db column(Student::_name)//指定类成员映射到数据库表中的列名
    std::string _name;
    #pragma db column(Student::_age)//指定类成员映射到数据库表中的列名
    odb::nullable<unsigned short> _age;
    #pragma db column(Student::_class_id)//指定类成员映射到数据库表中的列名
    unsigned long _class_id;
    #pragma db column(classes::_name)//指定类成员映射到数据库表中的列名
    std::string _class_name;
};


//ODB中的查询默认是将表的所有字段全部查询出来，而如果只想查询几个字段，就需要自己定义个查询
// 只查询学生姓名  ,   (?)  外部调用时传入的过滤条件
#pragma db view query("select name from Student" )
class all_name
{
    public:
    std::string name;
};
//odb -d mysql --generate-query --generate-schema --profile boost/date-time person.hxx

#include <string>
#include <memory> // std::auto_ptr
#include <cstdlib> // std::exit
#include <iostream>
#include <odb/database.hxx>
#include <odb/mysql/database.hxx>
#include "student.hxx"
#include "student-odb.hxx"

#include <gflags/gflags.h>


DEFINE_string(host,"127.0.0.1","这是mysql服务器的地址");
DEFINE_int32(port,0,"这是mysql服务器的端口");
DEFINE_string(db,"MyDB","这是使用的数据库名称");
DEFINE_string(user,"root","这是数据的用户名");
DEFINE_string(passw,"123456","这是mysql的密码");
DEFINE_string(cset,"utf8","这是mysql的字符集");
DEFINE_int32(max_pool,3,"这是mysql数据库连接池最大数据");



//要注意中间出错会抛异常，所以需要捕获
void insert_student(odb::mysql::database &db)
{
    try{
        //3.获取事务对象，开启事务
        odb::transaction trans(db.begin());
        //4.数据库相关操作，增删查改
        Student s1(1,"陶恩威",22,1);
        Student s2(2,"姚子怡",23,1);
        Student s3(3,"陶恩豪",22,1);
        Student s4(4,"张三",23,2);
        Student s5(5,"王五",24,2);
        db.persist(s1);
        db.persist(s2);
        db.persist(s3);
        db.persist(s4);
        db.persist(s5);
        //5.提交事务
        trans.commit();
    }catch(std::exception &e){
        std::cout<<"插入学生数据出错"<<e.what()<<std::endl;
    }
}
void insert_classes(odb::mysql::database &db)
{
    try{
        //3.获取事务对象，开启事务
        odb::transaction trans(db.begin());
        //4.数据库相关操作，增删查改
        Classes c1("计算机1班");
        Classes c2("计算机2班");
        db.persist(c1);
        db.persist(c2);
        //5.提交事务
        trans.commit();
    }catch(std::exception &e){
        std::cout<<"插入班级数据出错"<<e.what()<<std::endl;
    }
}
//查询一个学生的所有信息
Student select_student(odb::mysql::database &db)
{
    Student s;
     try{
        //3.获取事务对象，开启事务
        odb::transaction trans(db.begin());
        //4.数据库相关操作，增删查改
        typedef odb::query<Student> query;//给类型去别名，原类型太长了
        typedef odb::result<Student> result;
        result r(db.query<Student>(query::name=="陶恩威"));
        if(r.size()!=1)
        {
            std::cout<<"查询的结果数量不对\n";
            return Student();
        }
        s=*r.begin();
        //5.提交事务
        trans.commit();
    }catch(std::exception &e){
        std::cout<<"查询学生数据出错"<<e.what()<<std::endl;
    }
    return s;
}
//更新操作必须要先进行查询，要对查询到的结果进行更新
void update_student(odb::mysql::database &db,Student&s)
{
    try{
        //3.获取事务对象，开启事务
        odb::transaction trans(db.begin());
        //4.数据库相关操作，增删查改
        db.update(s);
        //5.提交事务
        trans.commit();
    }catch(std::exception &e){
        std::cout<<"更新学生数据出错"<<e.what()<<std::endl;
    }
}
//删除操作也是需要先进行查询，但是有一个接口可以直接实现
void remove_student(odb::mysql::database &db)
{
    try{
        //3.获取事务对象，开启事务
        odb::transaction trans(db.begin());
        //4.数据库相关操作，增删查改
        typedef odb::query<Student> query;
        db.erase_query<Student>(query::id==2);
        //5.提交事务
        trans.commit();
    }catch(std::exception &e){
        std::cout<<"更新学生数据出错"<<e.what()<<std::endl;
    }
}
void classes_student(odb::mysql::database &db)
{
    try{
        //3.获取事务对象，开启事务
        odb::transaction trans(db.begin());
        //4.数据库相关操作，增删查改
        typedef odb::query<Student_Class>query;
        typedef odb::result<Student_Class>result;
        result r(db.query<Student_Class>(query::classes::id==1));
        for(auto it=r.begin();it!=r.end();it++)
        {
            std::cout<<it->_id<<std::endl;
            std::cout<<it->_name<<std::endl;
            std::cout<<it->_sn<<std::endl;
            std::cout<<*it->_age<<std::endl;
            std::cout<<it->_class_id<<std::endl;
            std::cout<<it->_class_name<<std::endl;
        }
        //5.提交事务
        trans.commit();
    }catch(std::exception &e){
        std::cout<<"更新学生数据出错"<<e.what()<<std::endl;
    }
}
void allname(odb::mysql::database &db)
{
    try{
        //3.获取事务对象，开启事务
        odb::transaction trans(db.begin());
        //4.数据库相关操作，增删查改
        typedef odb::query<Student> query;
        typedef odb::result<all_name> result;
        result r(db.query<all_name>(query::id==1));
        for(auto it=r.begin();it!=r.end();it++)
        {
            std::cout<<it->name<<std::endl;
        }
        //5.提交事务
        trans.commit();
    }catch(std::exception &e){
        std::cout<<"查询所有学生姓名数据出错"<<e.what()<<std::endl;
    }
}
int main(int argc,char*argv[])
{
    //要想通过解析命令行参数来设置到定义的变量中，需要告诉可执行程序去处理解析命令行传入的参数
    google::ParseCommandLineFlags(&argc, &argv, true); 
    //上面的定义的参数名称并不是真正的全局变量，gflags内部会将名字前面统一添加FLAGS_
    /*ODB操作流程*/
    //1.构建线程池对象(构造连接池工厂配置对象)
    std::unique_ptr<odb::mysql::connection_pool_factory>cpf(new odb::mysql::connection_pool_factory(FLAGS_max_pool,0));
    //2.构建数据库操作database对象
    odb::mysql::database db(
        FLAGS_user,FLAGS_passw,FLAGS_db,FLAGS_host,FLAGS_port,"",FLAGS_cset,0,std::move(cpf)
    );
    //3.获取事务对象，开启事务
    //4.数据库相关操作，增删查改
    //5.提交事务

    //insert_student(db);
    //insert_classes(db);
    // Student s=select_student(db);
    // std::cout<<"姓名："<<s.getname()<<std::endl;
    // std::cout<<"年龄："<<*s.getage()<<std::endl;
    // std::cout<<"学号："<<s.getsn()<<std::endl;
    // std::cout<<"班级号："<<s.getclassid()<<std::endl;

    // //更新修改
    // s.setname("我不是陶恩威");
    // update_student(db,s);
    //remove_student(db);
    //classes_student(db);
    allname(db);
}
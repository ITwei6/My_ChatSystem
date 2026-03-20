
//gtest框架的头文件
#include <gtest/gtest.h>
int Add(int num1,int num2)
{
    return num1+num2;
}

TEST(test_name,add_test)
{
    //断言两个值是否相同
    ASSERT_EQ(Add(10,20),30);
    //断言第一个值是否小于第二个
    ASSERT_LT(Add(1,2),4);
}
TEST(test_name,string_test)
{
    std::string str="hello";
    ASSERT_EQ(str,"Hello");
}
int main(int argc,char*argv[])
{

    //首先需要初始化
    testing::InitGoogleTest(&argc, argv); 
    //初始化完后就调用所有测试用例
    return RUN_ALL_TESTS();
}
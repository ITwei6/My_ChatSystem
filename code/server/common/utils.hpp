#pragma
#include <fstream>
#include <string>
#include "logger.hpp"
#include <ctime>
#include <sstream>
#include <random>
#include <iomanip>
#include <atomic>
namespace tew_im{
    
    //生成一个唯一的id，UUID
    //主要有32位的十六进制数值表示，也就是16字节，先生成8个1字节的数字，转换位16进制，再生成一个8字节的序列号
    static std::string Uuid()
    {
        std::stringstream ss;
        // 1cl.首先构造一个机器随机数生成对象
        std::random_device rd;
        // 2.再以机器随机数为种子构建伪随机数对象
        std::mt19937 generator(rd());
        // 3.再构建限定随机数范围对象
        std::uniform_int_distribution<int> distribution(0, 255);
        // 4.生成8个1字节的随机数，每个随机数转换为16进制
        for (int i = 0; i < 8; i++)
        {
            if (i == 4 || i == 6)
                ss << "-";
            ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
            // 每一个随机数都要转换为16进制，并且有2个间距，不足的用0补位 如果随机数是1 则转换位16进制位01
        }
        // 5.再生成一个8字节的序列号，用静态变量，每次函数结束都不会修改变化
        static std::atomic<size_t> seq(0);
        size_t st = seq.fetch_add(1); // 每次序号都递增
        // 将该8字节的序号，每一个字节转换为16进制
        // 00 00 00 00 00 00 00 01
        ss << "-";
        for (int i = 7; i > 0; i--)
        {
            if (i == 5)
                ss << "-";
            ss << std::setw(2) << std::setfill('0') << std::hex << ((st >> (i * 8)) & 0xFF); // 获取一个字节的数
            // 每一个随机数都要转换为16进制，并且有2个间距，不足的用0补位 如果随机数是1 则转换位16进制位01
        }
        return ss.str();
    }
    //读取文件,就是将文件的内容读取到body变量中，相当于输入
    bool ReadFile(const std::string&filename,std::string&body)
    {
        std::ifstream ifs(filename,std::ios::binary| std::ios::in);
        if(!ifs.is_open())
        {
            ERROR_LOG("打开{}文件失败",filename);
            return false;
        }
        //打开成功，则要读取文件内容
        ifs.seekg(0, std::ios::end); // 将读取指针移动到文件数据末尾
        size_t size = ifs.tellg();   // 返回文件末尾到起始位置的偏移量，也就是文件大小
        ifs.seekg(0, std::ios::beg); // 再将读取指针移动到起始位置
        body.resize(size);
        ifs.read(&body[0],size);
        if(!ifs.good())
        {
            ERROR_LOG("从{}文件中读取数据失败",filename);
            ifs.close();
            return false;
        }
        //成功
        ifs.close();
        return true;
    }
    //写入数据，就是将变量body中存储的数据写入到文件中，相当于输出
    bool WriteFile(const std::string&filename,const std::string&body)
    {
        std::ofstream ofs(filename,std::ios::out|std::ios::binary|std::ios::trunc);
        if(!ofs.is_open())
        {
            ERROR_LOG("打开{}文件失败",filename);
            return false;
        }
        //打开成功，则写入
        ofs.write(&body[0],body.size());
        if(!ofs.good())
        {
            ERROR_LOG("往{}文件写入数据失败",filename);
            ofs.close();
            return false;
        }
        ofs.close();
        return true;
    }
}
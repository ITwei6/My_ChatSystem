main:main.cc
	g++ -std=c++11 $^ -o $@ -I/usr/include/jsoncpp -lcurl -lcrypto -ljsoncpp


# # 定义变量，方便修改
# CXX = g++
# CXXFLAGS = -std=c++11 -Wall -O2  # -Wall 开启所有警告，-O2 开启优化
# INCLUDES = -I/usr/include/jsoncpp  # jsoncpp 的头文件路径
# LDFLAGS = -lcurl -lcrypto -ljsoncpp  # 所有需要链接的库

# # 目标文件
# TARGET = main

# # 源文件
# SRCS = main.cc

# # 默认目标
# all: $(TARGET)

# # 生成可执行文件
# $(TARGET): $(SRCS)
# 	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SRCS) $(LDFLAGS)

# # 清理规则
# clean:
# 	rm -f $(TARGET) *.o
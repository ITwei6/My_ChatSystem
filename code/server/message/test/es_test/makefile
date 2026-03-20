# main : main.cc
# 	c++ main.cc -o main  -I../../../odb/ -I./ -L/usr/lib/x86_64-linux-gnu -lfmt -lspdlog -lgflags -lcpr -lelasticlient -ljsoncpp
# Makefile
TARGET = main
CXX = c++
# 头文件路径
INCLUDES = -I../../../odb/ -I./
# 库路径：优先 /usr/local/lib（elasticlient 和 jsoncpp19 所在路径）
LDFLAGS = -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu
# 链接库：elasticlient 后紧跟其依赖的 jsoncpp19，避免版本冲突
LIBS = -lfmt -lspdlog -lgflags -lcpr -lelasticlient /usr/local/lib/libjsoncpp.so.19

$(TARGET): main.cc
	$(CXX) $< -o $@ $(INCLUDES) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)
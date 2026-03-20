all:publisher consumer
publisher :publish.cc
	g++ -std=c++17 $^ -o $@  -lamqpcpp -lev -lspdlog -lpthread -lgflags -lfmt
consumer :consume.cc
	g++ -std=c++17 $^ -o $@  -lamqpcpp -lev -lspdlog -lpthread -lgflags -lfmt
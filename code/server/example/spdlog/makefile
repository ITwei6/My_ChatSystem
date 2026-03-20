all:main
sync:sync.cc
	g++ -std=c++17 $^ -o $@ -lspdlog -lpthread
async:async.cc
	g++ -std=c++17 $^ -o $@ -lspdlog -lpthread
main:main.cc
	g++ -std=c++17 $^ -o $@ -lspdlog -lpthread -lgflags
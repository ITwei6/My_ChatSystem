all::discovery register
discovery:discovery.cc main.pb.cc
	g++ -std=c++17 $^ -o$@ -lgflags -lspdlog -lpthread -letcd-cpp-api -lcpprest -lbrpc -lleveldb -lgflags -lssl -lcrypto -lprotobuf -ldl -g
register:register.cc   main.pb.cc
	g++ -std=c++17 $^ -o$@ -lgflags -lspdlog -lpthread -letcd-cpp-api -lcpprest -lbrpc -lleveldb -lgflags -lssl -lcrypto -lprotobuf -ldl -g
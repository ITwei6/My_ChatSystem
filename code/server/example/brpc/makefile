all:brpc_server brpc_client
brpc_server: server.cc main.pb.cc 
	g++ -std=c++17 $^ -o $@ -lbrpc -lleveldb -lgflags -lssl -lcrypto -lprotobuf -pthread -ldl
brpc_client: client.cc main.pb.cc 
	g++ -std=c++17 $^ -o $@ -lbrpc -lleveldb -lgflags -lssl -lcrypto -lprotobuf -pthread -ldl
all:publisher subscibler
publisher :publisher.cc
	g++ -std=c++17 $^ -o $@  -lamqpcpp -lev
subscibler :subscibler.cc
	g++ -std=c++17 $^ -o $@  -lamqpcpp -lev
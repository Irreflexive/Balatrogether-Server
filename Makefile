BINARIES=balatro_server test
CXXFLAGS=-std=c++11 -pthread -lcrypto -lssl

balatro_server: main.o server.o encrypt.o
	$(CXX) $(CXXFLAGS) $^ -o $@

server.o: server.cpp server.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ server.cpp

test: test_client.o encrypt.o server.hpp
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(BINARIES) *.o
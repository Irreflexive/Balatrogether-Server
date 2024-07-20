BINARIES=balatro_server test
CXXFLAGS=-std=c++11 -pthread -lcrypto

balatro_server: main.o server.o encrypt.o server.hpp
	$(CXX) $(CXXFLAGS) $^ -o $@

test: test_client.o encrypt.o server.hpp
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(BINARIES) *.o
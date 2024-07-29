BINARIES=balatro_server test
CXXFLAGS=-std=c++11 -pthread -lcrypto -lssl

balatro_server: main.o server.o encrypt.o player.o preq.o
	$(CXX) $(CXXFLAGS) $^ -o $@

test: test_client.o encrypt.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(BINARIES) *.o
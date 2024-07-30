BINARIES=balatro_server test
CXXFLAGS=-std=c++11
LIBS=-pthread -lcrypto -lssl

balatro_server: main.o server.o encrypt.o player.o preq.o config.o
	$(CXX) $(CXXFLAGS) $(LIBS) $^ -o $@

test: test_client.o encrypt.o
	$(CXX) $(CXXFLAGS) $(LIBS) $^ -o $@

clean:
	rm -f $(BINARIES) *.o
BINARIES=main test
CXXFLAGS=-std=c++11 -pthread -lcrypto

main: main.o server.o encrypt.o
	$(CXX) $(CXXFLAGS) $^ -o $@

test: test_client.o encrypt.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(BINARIES) *.o
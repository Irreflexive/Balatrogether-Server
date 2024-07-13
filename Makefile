BINARIES=main test
CXXFLAGS=-std=c++11 -pthread

main: main.o server.o
	$(CXX) $(CXXFLAGS) $^ -o $@

test: test_client.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(BINARIES) *.o
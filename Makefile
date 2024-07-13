BINARIES=main
CXXFLAGS=-std=c++11 -pthread -I/usr/include/openssl/ -lssl -lcrypto

all: $(BINARIES)

main: main.o server.o encryption.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(BINARIES) *.o
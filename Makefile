BINARIES=main
CXXFLAGS=-std=c++11 -pthread

all: $(BINARIES)

main: main.o server.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(BINARIES) *.o
CXX = g++
CXXFLAGS = -Wall -O3 -std=c++17 -pthread

all: hftengine

hftengine: server.o concurrency.o orderbook.o memorypool.o
	$(CXX) $(CXXFLAGS) -o hftengine server.o concurrency.o orderbook.o memorypool.o

server.o: server.cpp
	$(CXX) $(CXXFLAGS) -c server.cpp

concurrency.o: concurrency.cpp
	$(CXX) $(CXXFLAGS) -c concurrency.cpp

orderbook.o: orderbook.cpp
	$(CXX) $(CXXFLAGS) -c orderbook.cpp

memorypool.o: memorypool.cpp
	$(CXX) $(CXXFLAGS) -c memorypool.cpp

clean:
	rm -f *.o hftengine

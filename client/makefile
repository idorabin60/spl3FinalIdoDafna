# Compiler and flags
CXX := g++
CFLAGS := -c -Wall -Weffc++ -g -std=c++11 -Iinclude
LDFLAGS := -lboost_system -lpthread

# Targets
all: StompWCIClient run

StompWCIClient: bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/StompFrame.o bin/StompProtocol.o
	$(CXX) -o bin/StompWCIClient bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/StompFrame.o bin/StompProtocol.o $(LDFLAGS)

run: StompWCIClient
	@echo "Running StompWCIClient..."
	./bin/StompWCIClient

# Rules for object files
bin/ConnectionHandler.o: src/ConnectionHandler.cpp include/ConnectionHandler.h
	$(CXX) $(CFLAGS) -o bin/ConnectionHandler.o src/ConnectionHandler.cpp

bin/StompClient.o: src/StompClient.cpp include/StompProtocol.h include/StompFrame.h
	$(CXX) $(CFLAGS) -o bin/StompClient.o src/StompClient.cpp

bin/event.o: src/event.cpp
	$(CXX) $(CFLAGS) -o bin/event.o src/event.cpp

bin/StompFrame.o: src/StompFrame.cpp include/StompFrame.h
	$(CXX) $(CFLAGS) -o bin/StompFrame.o src/StompFrame.cpp

bin/StompProtocol.o: src/StompProtocol.cpp include/StompProtocol.h include/StompFrame.h
	$(CXX) $(CFLAGS) -o bin/StompProtocol.o src/StompProtocol.cpp

# Clean target
.PHONY: clean
clean:
	rm -f bin/*

all: build

build: ./src/server.cpp ./src/client.cpp ./src/DropperServer.cpp ./src/SocketManager.cpp ./src/ClientData.cpp ./include/DropperServer.h ./include/DropperClient.h ./include/SocketManager.h ./include/ClientData.h
	rm -rf ./server
	rm -rf ./client
	g++ ./src/server.cpp ./src/DropperServer.cpp ./src/SocketManager.cpp ./src/ClientData.cpp -o ./server -lpthread -std=c++11
	g++ ./src/client.cpp ./src/DropperClient.cpp ./src/SocketManager.cpp -o ./client -lpthread -std=c++11

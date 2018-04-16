all: build

build: ./src/server.cpp ./src/DropboxServer.cpp ./src/DropboxClient.cpp ./src/client.cpp ./include/DropboxServer.h ./include/DropboxClient.h
	rm -rf build
	mkdir build
	g++ ./src/server.cpp ./src/DropboxServer.cpp ./src/DropboxClient.cpp -o ./build/server -lpthread
	g++ ./src/client.cpp -o ./build/client

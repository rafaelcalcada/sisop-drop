all: build

build: ./src/server.cpp ./src/client.cpp ./src/front-end.cpp ./src/DServer.cpp ./src/DSocket.cpp ./src/DClient.cpp ./src/DMessage.cpp ./src/DFile.cpp ./src/DFrontEnd.cpp ./include/DServer.h ./include/DClient.h ./include/DSocket.h ./include/DClient.h ./include/DMessage.h ./include/DFile.h ./include/DFrontEnd.h
	rm -rf ./server
	rm -rf ./client
	rm -rf ./front-end
	g++ ./src/server.cpp ./src/DServer.cpp ./src/DSocket.cpp ./src/DClient.cpp ./src/DMessage.cpp ./src/DFile.cpp -o ./server -lpthread -std=c++11
	g++ ./src/client.cpp ./src/DClient.cpp ./src/DSocket.cpp ./src/DMessage.cpp ./src/DFile.cpp -o ./client -lpthread -std=c++11
	g++ ./src/front-end.cpp ./src/DFrontEnd.cpp ./src/DSocket.cpp ./src/DMessage.cpp -o ./front-end -lpthread -std=c++11

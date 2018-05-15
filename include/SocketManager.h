#ifndef __SOCKETMANAGER_H
#define __SOCKETMANAGER_H

#include <iostream>
#include <string>
#include <cstring>

#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

const int MAX_DEVICES = 2;
const int BUFFER_SIZE = 1024;

using namespace std;

class SocketManager {
	
	private:
	
		struct sockaddr_in address; // endereço do socket que criou o SocketManager
		struct sockaddr_in sender_address; // endereço do último dispositivo de onde o SocketManager recebeu uma mensagem com receive()
		struct sockaddr_in* destination_address; // endereço do dispositivo para onde será enviada uma mensagem com send()
		socklen_t sender_address_size;
		int socket_desc;		
		int port;
		int error_flag;
		string ip;
	
	public:

		SocketManager(); // construtor padrão: apenas cria o socket, não faz o bind (método usado pelo cliente)
		SocketManager(string ip, int port); // cria o socket e faz o bind com o endereço e porta informados (método usado pelo servidor)
		bool bad() { return error_flag; }
		bool prepareToSend(string ip, int port); // configura IP e porta do socket para onde será enviada uma mensagem com send();
		bool receive(char** message, socklen_t* message_size); // encapsulamento de recvfrom()
		bool receiveFile(char* file_buf, int file_size); // encapsulamento de recvfrom() para arquivos
		bool send(const char* message, int message_size); // encapsulamento de sendto()
		bool closeSocket();
		void changeDestinationPort(int new_port) { destination_address->sin_port = htons(new_port); } // muda apenas a porta do endereço de destino
		int getSenderPort() { return (int) ntohs(sender_address.sin_port); }
		string getSenderIP() { return string(inet_ntoa(sender_address.sin_addr)); }
		
};

#endif

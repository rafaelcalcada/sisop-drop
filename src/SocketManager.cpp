#include "../include/SocketManager.h"

SocketManager::SocketManager() {
	
	this->ip = "invalid";
	this->port = -1;
	this->error_flag = false;
	this->sender_address_size = sizeof(struct sockaddr_in);
	this->destination_address = NULL;
	
	this->socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
	
	if(socket_desc == -1) {
	
		error_flag = true;
		cout << "Erro ao criar socket." << endl;
		
	}
	
}


SocketManager::SocketManager(string ip, int port) {
	
	this->ip = ip;
	this->port = port;
	this->error_flag = false;
	this->sender_address_size = sizeof(struct sockaddr_in);
	this->destination_address = NULL;
	
	this->socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
	
	if(socket_desc == -1) {
	
		error_flag = true;
		cout << "Erro ao criar socket." << endl;
		
	} else {
				
		address.sin_family = AF_INET;
		address.sin_port = htons(port);
		
		if(ip == "localhost") {

			address.sin_addr.s_addr = INADDR_ANY;

		} else {

			int flag = inet_aton(ip.c_str(), &(address.sin_addr));

			if(flag == 0) { // endereço inválido
		
				error_flag = true;
				cout << "Erro. Não foi possível converter a string em endereço IP." << endl;
				this->closeSocket();
		
			}

		}
		
		if(!error_flag) { // tudo ok até aqui

			int flag2 = bind(socket_desc, (struct sockaddr *) &address, sizeof(struct sockaddr));
				
			if (flag2 < 0) { // erro durante o bind

				error_flag = true;
				cout << "Erro ao vincular socket a uma porta e endereço." << endl;
				this->closeSocket();

			}
			
		}
		
	}
	
}

bool SocketManager::closeSocket() {
	
	int close_flag = close(socket_desc);
	
	if(close_flag == 0 || close_flag == EBADF) return true;
	else return false;
	
}

bool SocketManager::receive(char** message, socklen_t* message_size) {
	
	if(error_flag == false) { // nenhum erro até então
		
		char* buffer = new char[BUFFER_SIZE];
				
		int msg_sz = recvfrom(socket_desc, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sender_address, &sender_address_size);
		
		if(msg_sz < 0) {
			
			cout << "Erro durante recebimento da mensagem." << endl;
			return false;
			
		} else {
			
			char* msg = new char(msg_sz);
			memcpy(msg,buffer,msg_sz);
			delete[] buffer;
			*message = msg;
			*message_size = msg_sz;
			return true;
			
		}
		
	} else { // houve erros antes da invocação do método
		
		return false;
		
	}
	
	
}

bool SocketManager::receiveFile(char* file_buf, int file_size) {
	
	if(error_flag == false) { // nenhum erro até então
					
		int msg_sz = recvfrom(socket_desc, file_buf, file_size, 0, (struct sockaddr *) &sender_address, &sender_address_size);
		
		if(msg_sz < 0) {
			
			cout << "Erro durante recebimento da mensagem." << endl;
			return false;
			
		} else {
			
			return true;
			
		}
		
	} else { // houve erros antes da invocação do método
		
		return false;
		
	}
	
	
}

bool SocketManager::prepareToSend(string ip, int port) {

	this->destination_address = new struct sockaddr_in;

	destination_address->sin_family = AF_INET;
	destination_address->sin_port = htons(port);
		
	int flag = inet_aton(ip.c_str(), &(destination_address->sin_addr));

	if(flag == 0) { // endereço inválido
	
		this->error_flag = true;
		cout << "Erro ao preparar para envio. Não foi possível converter a string em endereço IP." << endl;
		delete destination_address;
		destination_address = NULL;
		this->closeSocket();
		return false;
	
	}

	return true;

}

bool SocketManager::send(const char* message, int message_size) {
	
	if(error_flag == false && destination_address != NULL) {

		int send_ok = sendto(socket_desc, (const void*) message, message_size, 0, (struct sockaddr *) destination_address, sizeof(struct sockaddr));

		if(send_ok < 0) {
			
			cout << "Erro durante envio da mensagem." << endl;
			return false;
			
		} else return true;
		
	} else {
		
		cout << "Erro ao enviar mensagem." << endl;
		return false;
		
	}
	
	
}

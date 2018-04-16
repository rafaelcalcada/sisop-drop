#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

using namespace std;

const int bufferSize = 1024;

int main(int argc, char* argv[]) {

	if(argc < 4) {

		cout << "Uso: ./client [usuario] [IP_servidor] [porta_servidor]" << endl;
		return 0;

	} else {

		// argv[1] => nome do usuário
		// argv[2] => IP do servidor
		// argv[3] => Porta do servidor
		cout << "Conectando usuario \"" << argv[1] << "\" ao servidor " << argv[2] << ":" << argv[3] << " ..." << endl;

		// Converte argv[2] no IP do servidor
		in_addr_t ipsrv = inet_addr(argv[2]);
		if(ipsrv == -1) {
			cout << "Não foi possível converter " << argv[2] << " no IP do servidor" << endl;
		}
		
		// Criação do socket do cliente
		int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	    if (clientSocket == -1) {
			cout << "Erro ao criar socket UDP do cliente." << endl;
			return -1;
		}
			
		// Inicialização das estruturas do socket
		struct sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(atoi(argv[3]));
		serverAddress.sin_addr.s_addr = ipsrv;
		bzero(&(serverAddress.sin_zero), 8);    

		// Cria uma mensagem
		char* buffer = new char[bufferSize];
		bzero(buffer,bufferSize);
		strcpy(buffer, "newsession ");
		strcat(buffer, argv[1]);

		// Envia a mensagem
		if(sendto(clientSocket, buffer, strlen(buffer), 0, (const struct sockaddr *) &serverAddress, sizeof(struct sockaddr_in)) < 0) {

			cout << "Erro ao enviar pedido de conexão para o servidor." << endl;
			close(clientSocket);
			return -1;

		}

		// Espera resposta do servidor
		struct sockaddr_in from; socklen_t socketLength = sizeof(struct sockaddr_in);
		bzero(buffer, bufferSize);
		if(recvfrom(clientSocket, buffer, bufferSize, 0, (struct sockaddr *) &from, &socketLength)) {

			cout << "> Resposta do servidor: " << buffer << endl;

		}

		close(clientSocket);
		return 0;

	}

}

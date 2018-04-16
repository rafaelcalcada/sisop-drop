#include <iostream>
#include <list>
#include <string>
#include "DropboxClient.h"

#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>

using namespace std;

const int bufferSize = 1024;

class DropboxServer {

	private:
		list<DropboxClient*> clients; // lista encadeada de clientes do servidor
		static const int listeningPort = 50000; // número da porta que espera por conexões de clientes

	public:
		DropboxServer();	// construtor da classe. chama fillClientsList(). após, cria uma thread que espera por requisições de clientes
		bool fillClientsList(); // para cada diretório de cliente (dentro do diretório do servidor), cria um cliente e o coloca na lista
		bool newClientSession(string uid, struct sockaddr_in* clientAddress); // cria uma sessão com um cliente (limite: 2 sessões)
		bool receiveFile(string uid, string fileName); // recebe um arquivo e salva na pasta virtual do usuário
		bool sendFile(string uid, string fileName); // envia um arquivo da pasta virtual para o cliente
		bool syncServer(string uid); // sincroniza os arquivos entre cliente e servidor

};

// thread que fica aguardando requisições de clientes
void* listen(void* args);

// struct para passar os argumentos da thread listen
typedef struct listen_args {
	int socketId;
	void* serverPointer;
} listenArgs;

#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <thread>

#include "SocketManager.h"
#include "ClientData.h"

#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>

using namespace std;

const int SERVER_PORT_NUMBER = 50000;
const int MAX_CONNECTIONS = 1000;

enum conn_id_status { AVAILABLE, UNAVAILABLE, USED };

class DropperServer {
	
	private:
	
		list<ClientData*> clients_list;
		SocketManager* server_socket;
		thread* connections[MAX_CONNECTIONS];
		string homedir;
		int connection_id[MAX_CONNECTIONS]; // vetor que armazena o status de cada id de conexão (status = AVAILABLE, UNAVAILABLE ou USED)
		bool error_flag;
		
	public:
	
		DropperServer(); // construtor padrão. inicializa um socket para o servidor e preenche a lista de clientes OK
		ClientData* findClient(string client_name); // retorna NULL se não encontra o cliente na lista OK
		void listen(); // aguarda pedidos de conexão de clientes OK
		void messageProcessing(ClientData* client, SocketManager* client_socket, int conn_id); // trata mensagens recebidas do cliente OK
		bool acceptConnection(ClientData* client, string client_ip, int client_port); // aceita conexões. cria uma thread para cada nova conexão aceita  OK
		bool initialize(); // inicializa a lista de clientes e cria diretório do servidor, caso não exista OK
		bool bad() { return error_flag; } // OK
		bool newConnection(ClientData* client); // OK
		bool receiveFile(string client_name, string file_path);
		bool sendFile(string client_name, string file_path);
		bool synchronize(string client_name);
		int getConnectionId(); // OK
	

};

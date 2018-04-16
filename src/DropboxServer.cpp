#include "../include/DropboxServer.h"

extern const int MAX_DEVICES;

DropboxServer::DropboxServer() {

	if(this->fillClientsList()) cout << "Lista de clientes preenchida com sucesso." << endl;
	else return;

	// tenta criar um socket UDP
	int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket == -1) cout << "Erro ao criar socket UDP do servidor." << endl;

	// Inicializa estrutura do socket
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(listeningPort);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serverAddress.sin_zero), 8);    

	// tenta vincular o socket a uma porta e endereço	 
	if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(struct sockaddr)) < 0) {

		cout << "Erro ao vincular socket a uma porta e endereço. Experimente fechar o terminal e re-executar o servidor." << endl;
		close(serverSocket);

	} else {

		// se a vinculação é bem sucedida, cria uma tread que espera por requisições de clientes	
		pthread_t listeningThread;
		listenArgs* largs = new listenArgs; // os argumentos da thread são passados através de uma struct
		largs->socketId = serverSocket;
		largs->serverPointer = (void*) this;
		if(pthread_create(&listeningThread, NULL, listen, (void*) largs) != 0) {
			cout << "Erro ao criar thread que espera por conexões de clientes." << endl;
			close(serverSocket);
		}

		cout << "Servidor inicializado com sucesso." << endl;

		// Aguarda comandos do usuário
		string command;
		cout << "Para desligar o servidor, digite \"quit\" e pressione enter." << endl << "> ";
		while(cin >> command) {

			if(command.compare("quit") == 0) {
				break;
			} else {
				cout << "Comando inválido." << endl;
				cout << "Para desligar o servidor, digite \"quit\" e pressione enter." << endl << "> ";
			}

		}

		// Fecha socket
		close(serverSocket);
		cout << "Servidor desligado com sucesso." << endl;

	}

}

void* listen(void* args) {

	char* buffer = new char[bufferSize];
	int flag;

	// recupera os argumentos passados através da struct
	listenArgs* largs = (listenArgs*) args;
	int serverSocket = largs->socketId;
	DropboxServer* server = (DropboxServer*) largs->serverPointer;

	// Inicializa uma estrutura para enviar resposta ao cliente
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLength = sizeof(struct sockaddr_in);

	// Fica esperando requisições dos clientes
	while(true) {

		bzero(buffer, bufferSize);
		flag = recvfrom(serverSocket, buffer, bufferSize, 0, (struct sockaddr *) &clientAddress, &clientAddressLength);
		if (flag < 0) cout << "Erro durante o recebimento de requisição do cliente" << endl;

		cout << "Recebeu requisição: '" << buffer << "'\n> ";
		cout.flush(); // força a impressão imediata do "cout" na tela
		
		// PROCESSAMENTO DA REQUISIÇÃO
		string request(buffer);

			// requisição é pedido de nova sessão para um cliente
			if(request.find("newsession") != string::npos) {
				
				string uid = request.substr(11); // remove os 11 primeiros caracteres ("newsession ")
				bool connectionAccepted = server->newClientSession(uid, &clientAddress);

			}

			// TODO: Implementar requisições:	- upload [filename]
			//									- download [filename]
			//									- list_server
			//									- exit

		// Envia uma resposta de confirmação ao cliente
		bzero(buffer, bufferSize);
		strcpy(buffer, "Conexão aceita");
		flag = sendto(serverSocket, buffer, strlen(buffer), 0,(struct sockaddr *) &clientAddress, sizeof(struct sockaddr));
		if (flag  < 0) cout << "Erro durante o envio de confirmação de conexão ao cliente." << endl;

	}

}

bool DropboxServer::fillClientsList() {

	// Verifica se o diretório do servidor já foi criado
	struct stat st;
	if(stat("/tmp/sisop-drop", &st) != -1) { // Caso 1: já foi criado

		cout << "Verificando clientes..." << endl;

		// Inicializa estruturas para percorrer o diretório do servidor
		DIR* dir = opendir("/tmp/sisop-drop");
		struct dirent* de;

		// Percorre todas as entradas do diretório do servidor
		while((de = readdir(dir)) != NULL) {

			// ignora as entradas de diretório "." e ".."
			if(strcmp(de->d_name,".") == 0 || strcmp(de->d_name,"..") == 0) continue;

			// constrói o path da entrada do diretório encontrada
			struct stat destat;
			char entrypath[1024];
			bzero(entrypath,1024);
			strcat(entrypath,"/tmp/sisop-drop/");
			strcat(entrypath,de->d_name);

			// testa se a entrada é um diretório ou não
			stat(entrypath,&destat);
			if(S_ISDIR(destat.st_mode)) {
				// se encontrou o diretório de um cliente, cria este cliente e o adiciona à lista
				string newUid(de->d_name);
				DropboxClient* newClient = new DropboxClient(newUid);
				clients.push_back(newClient);
				cout << "Cliente '" << newUid << "' adicionado com sucesso." << endl;
			}

		}
	
		return true;

	} else { // Caso 2: não foi criado

		cout << "Diretório do servidor não existe. Criando diretório..." << endl;

		if(mkdir("/tmp/sisop-drop", 0777) == 0) {
			cout << "Diretório do servidor criado com sucesso." << endl;
			return true;
		} else {
			cout << "Erro ao criar diretório do servidor." << endl;
			return false;
		}

	}

}

bool DropboxServer::newClientSession(string uid, struct sockaddr_in* clientAddress) {

	// Percorre a lista de clientes, pesquisando se o cliente já existe
	bool found = false;
	list<DropboxClient*>::iterator it;
	for(it = clients.begin(); it != clients.end(); ++it) {
		if((*it)->getUid().compare(uid) == 0) {
			found = true;
			break;
		}
	}

	// cliente já existe. cria uma nova sessão (se nº sessões < 2)
	if(found) {

		if((*it)->getActiveSessions() < MAX_DEVICES) {

			// verifica se entre as sessões ativas já se encontra o dispositivo atual
			bool found = false;
			struct sockaddr_in** devices = (*it)->getDevices();
			for(int i = 0; i < MAX_DEVICES; i++) {
				if(devices[i] != NULL) {
					if(devices[i]->sin_addr.s_addr == clientAddress->sin_addr.s_addr) {
						found = true;
						break;
					}
				}
			}

			if(found) 
				cout << "Nova sessão não estabelecida. Já existe uma sessão aberta pelo usuário '" << uid << "' através do dispositivo '" <<
					 inet_ntoa(clientAddress->sin_addr) << "'.\n> ";
			else {
				
				struct sockaddr_in* cliaddr = new struct sockaddr_in;
				*cliaddr = *clientAddress;
				bool sessionCreated = (*it)->addDevice(cliaddr);

				if(sessionCreated) {

					cout << "Sessão com o cliente '" << uid << "' estabelecida com sucesso.\n> ";

					// TODO: SINCRONIZAR DIRETÓRIO DO CLIENTE NO SERVIDOR COM O DIRETÓRIO LOCAL DO CLIENTE
					// implementar DropboxServer::syncServer(string uid)

				} else { cout << "Erro ao iniciar sessão com o cliente. Causa provável: limite de sessões excedido.\n> "; return false; }

			}

		} else {

			cout << "Erro ao iniciar sessão com o cliente. Número de sessões excedido.\n> ";

		}

	// cliente não existe. cria um novo cliente, adiciona à lista e cria nova sessão
	} else {

		// cria diretório para o cliente
		char path[1024];
		strcat(path,"/tmp/sisop-drop/");
		strcat(path,uid.c_str());
		if(mkdir(path, 0777) == 0) {
			cout << "Diretório do cliente criado com sucesso no servidor.\n> " << endl;
		} else {
			cout << "Erro ao criar diretório do cliente." << endl;
			return false;
		}		

		// adiciona cliente à lista
		DropboxClient* newClient = new DropboxClient(uid);
		clients.push_back(newClient);

		// cria uma nova sessão
		struct sockaddr_in* cliaddr = new struct sockaddr_in;
		*cliaddr = *clientAddress;
		bool sessionCreated = newClient->addDevice(cliaddr);

		if(sessionCreated) cout << "Sessão com o cliente '" << uid << "' estabelecida com sucesso.\n> ";
		else { cout << "Erro ao iniciar sessão com o cliente.\n> "; return false; }

	}

	cout.flush();
	return true;

}

bool DropboxServer::receiveFile(string uid, string fileName) {

	cout << "Chamou recebimento de arquivo." << endl;

	return true;

}

bool DropboxServer::sendFile(string uid, string fileName) {

	cout << "Chamou envio de arquivo." << endl;

	return true;

}

bool DropboxServer::syncServer(string uid) {

	cout << "Chamou sincronizacao de servidor." << endl;

}

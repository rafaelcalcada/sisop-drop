#include "../include/DServer.h"

DServer::DServer()
{
	serverSocket = new DSocket();
	if(serverSocket->isOpen()) {
		bool bindOk = serverSocket->bindSocket("localhost",SERVER_PORT);
		if(bindOk) {
			for(int i = 0; i < MAX_CONNECTIONS; i++) ports[i] = AVAILABLE;
			this->initialize();
			this->serverId = 0;
			serverId--; // serverId é unsigned, decrementá-lo causa range overflow e faz serverId igual ao maior inteiro possível
			struct ReplicaManager* primary = new struct ReplicaManager;
			primary->address = serverSocket->getSocketIp();
			primary->id = serverId;
			primary->primary = true;
			replicaManagers.push_back(primary);
			this->isPrimary = true;
			this->electionRunning = false;
			isWorking = true; }
		else {
			cout << "DServer::DServer() - Erro. Chamada de serverSocket->bind() retornou código de erro." << endl;
			serverSocket->closeSocket();
			isWorking = false; } }
	else {
		cout << "DServer::DServer() - Erro. Chamada de serverSocket->isOpen() retornou código de erro." << endl;
		isWorking = false; }
}

void DServer::initialize()
{
	this->homeDir = string("/tmp");
	string serverPath = homeDir + "/dbox-server";
	struct stat st;
	if(stat(serverPath.c_str(), &st) != -1) {
		DIR* dir = opendir(serverPath.c_str());
		struct dirent* de;
		while((de = readdir(dir)) != NULL) {
			if(strcmp(de->d_name,".") == 0 || strcmp(de->d_name,"..") == 0) continue;
			struct stat destat;
			string entryPath = serverPath + "/" + string(de->d_name);
			stat(entryPath.c_str(),&destat);
			if(S_ISDIR(destat.st_mode)) {
				string clientName(de->d_name);
				DClient* newClient = new DClient(clientName, false);
				clients.push_back(newClient);
				newClient->fillFilesList("/dbox-server"); } } }
	else if(mkdir(serverPath.c_str(), 0777) != 0) cout << "DServer::initialize() - Erro. Não conseguiu criar diretório raiz do servidor." << endl;
}

DClient* DServer::findClient(string clientName)
{
	list<DClient*>::iterator it;
	for(it = clients.begin(); it != clients.end(); ++it) {
		DClient* client = *(it);
		if(client->getName() == clientName) return client; }
	return NULL;
}

void DServer::startBackup(DSocket* connSocket)
{
	string backupIp = string(inet_ntoa(connSocket->getDestinationIp()));
	cout << "Inicializando novo servidor backup na máquina " << backupIp << "." << endl;
	DMessage* start = NULL;
	bool startOk = connSocket->receive(&start);
	if(startOk && string(start->get()) == "updatestart") {
		struct ReplicaManager* rm = new struct ReplicaManager;
		rm->address = connSocket->getDestinationIp();
		bool fail = false;
		unsigned int newServerId = this->serverId;
		list<ReplicaManager*>::iterator it;
		for(it = replicaManagers.begin(); it != replicaManagers.end(); ++it) {
			ReplicaManager* rmit = *(it);
			//if(string(inet_ntoa(rmit->address)) == backupIp) { fail = true; break; }
			if(rmit->id < newServerId) newServerId = rmit->id; }
		//if(fail) {
			//cout << "DServer::startBackup() - Falha ao iniciar novo servidor backup.\nMáquina " << backupIp << " já está rodando um servidor." << endl;
			//connSocket->reply(new DMessage("startfail")); }
		//else {
			rm->id = --newServerId;			
			replicaManagers.push_back(rm);
			cout << "Servidor backup inicializado com sucesso." << endl;
			DMessage* backack = new DMessage("backack " + to_string(rm->id));
			bool messageSent = connSocket->reply(backack);
			if(messageSent) {
				DMessage* listRequest = NULL;
				bool listRequestReceived = connSocket->receive(&listRequest);
				if(listRequestReceived) {
					for(it = replicaManagers.begin(); it != replicaManagers.end(); ++it) {
						ReplicaManager* rmit = *(it);
						string rmip = (rmit->primary == true) ? "primary" : string(inet_ntoa(rmit->address));
						DMessage* listItem = new DMessage("li:" + rmip + "," + to_string(rmit->id));
						bool listItemSent = connSocket->reply(listItem);
						if(!listItemSent) {
							cout << "Erro ao enviar lista de servidores para novo servidor backup." << endl;
							break;
						} else {
							DMessage* ok = NULL;
							bool waitOk = connSocket->receive(&ok);
							if(!waitOk || string(ok->get()) != "ok") {
								cout << "Erro ao enviar lista de servidores para novo servidor backup." << endl;
								break; } } }
					bool fullListSent = connSocket->reply(new DMessage("fullListSent"));
					if(!fullListSent) cout << "Erro inesperado." << endl;
					//else cout << "Lista de backup servers enviada com sucesso." << endl;
					}
				listRequestReceived = connSocket->receive(&listRequest);
				if(listRequestReceived) {
					list<DClient*>::iterator it2;
					for(it2 = clients.begin(); it2 != clients.end(); ++it2) {
						DClient* client = *(it2);
						DMessage* listItem = new DMessage("client:" + client->getName());
						bool listItemSent = connSocket->reply(listItem);
						if(!listItemSent) {
							cout << "Erro ao enviar lista de clientes para novo servidor backup." << endl;
							break;
						} else {
							DMessage* ok = NULL;
							bool waitOk = connSocket->receive(&ok);
							if(!waitOk || string(ok->get()) != "ok") {
								cout << "Erro ao enviar lista de clients para novo servidor backup." << endl;
								break; } } }
					bool fullListSent = connSocket->reply(new DMessage("fullListSent"));
					if(!fullListSent) cout << "Erro inesperado." << endl;
					//else cout << "Lista de clientes enviada com sucesso." << endl;
					}
				listRequestReceived = connSocket->receive(&listRequest);
				if(listRequestReceived) {
					list<DSocket*>::iterator it3;
					for(it3 = clisocks.begin(); it3 != clisocks.end(); ++it3) {
						DSocket* sock = *(it3);
						DMessage* listItem = new DMessage(string(inet_ntoa(sock->getDestinationIp())) + ":" + to_string(ntohs(sock->getDestinationPort())));
						bool listItemSent = connSocket->reply(listItem);
						if(!listItemSent) {
							cout << "Erro ao enviar lista de conexões para novo servidor backup." << endl;
							break;
						} else {
							DMessage* ok = NULL;
							bool waitOk = connSocket->receive(&ok);
							if(!waitOk || string(ok->get()) != "ok") {
								cout << "Erro ao enviar lista de conexões para novo servidor backup." << endl;
								break; } } }
					bool fullListSent = connSocket->reply(new DMessage("fullListSent"));
					if(!fullListSent) cout << "Erro inesperado." << endl;
					//else cout << "Lista de conexões enviada com sucesso." << endl;
					} } }// }
	closeSocketAndFreePort(connSocket);
}

void DServer::updateBackup(DSocket* connSocket)
{
	string backupIp = string(inet_ntoa(connSocket->getDestinationIp()));
	DMessage* start = NULL;
	bool startOk = connSocket->receive(&start);
	if(startOk && string(start->get()) == "updatestart") {
		DMessage* backack = new DMessage("updateack");
		bool messageSent = connSocket->reply(backack);
		if(messageSent) {
			DMessage* listRequest = NULL;
			bool listRequestReceived = connSocket->receive(&listRequest);
			if(listRequestReceived) {
				list<ReplicaManager*>::iterator it;
				for(it = replicaManagers.begin(); it != replicaManagers.end(); ++it) {
					ReplicaManager* rmit = *(it);
					string rmip = (rmit->primary == true) ? "primary" : string(inet_ntoa(rmit->address));
					DMessage* listItem = new DMessage("li:" + rmip + "," + to_string(rmit->id));
					bool listItemSent = connSocket->reply(listItem);
					if(!listItemSent) {
						cout << "Erro ao enviar lista de servidores para novo servidor backup." << endl;
						break;
					} else {
						DMessage* ok = NULL;
						bool waitOk = connSocket->receive(&ok);
						if(!waitOk || string(ok->get()) != "ok") {
							cout << "Erro ao enviar lista de servidores para novo servidor backup." << endl;
							break; } } }
				bool fullListSent = connSocket->reply(new DMessage("fullListSent"));
				if(!fullListSent) cout << "Erro inesperado." << endl; }
			listRequestReceived = connSocket->receive(&listRequest);
			if(listRequestReceived) {
				list<DClient*>::iterator it2;
				for(it2 = clients.begin(); it2 != clients.end(); ++it2) {
					DClient* client = *(it2);
					DMessage* listItem = new DMessage("client:" + client->getName());
					bool listItemSent = connSocket->reply(listItem);
					if(!listItemSent) {
						cout << "Erro ao enviar lista de clientes para novo servidor backup." << endl;
						break;
					} else {
						DMessage* ok = NULL;
						bool waitOk = connSocket->receive(&ok);
						if(!waitOk || string(ok->get()) != "ok") {
							cout << "Erro ao enviar lista de clients para novo servidor backup." << endl;
							break; } } }
				bool fullListSent = connSocket->reply(new DMessage("fullListSent"));
				if(!fullListSent) cout << "Erro inesperado." << endl; }
				listRequestReceived = connSocket->receive(&listRequest);
				if(listRequestReceived) {
					list<DSocket*>::iterator it3;
					for(it3 = clisocks.begin(); it3 != clisocks.end(); ++it3) {
						DSocket* sock = *(it3);
						DMessage* listItem = new DMessage(string(inet_ntoa(sock->getDestinationIp())) + ":" + to_string(ntohs(sock->getDestinationPort())));
						bool listItemSent = connSocket->reply(listItem);
						if(!listItemSent) {
							cout << "Erro ao enviar lista de conexões para novo servidor backup." << endl;
							break;
						} else {
							DMessage* ok = NULL;
							bool waitOk = connSocket->receive(&ok);
							if(!waitOk || string(ok->get()) != "ok") {
								cout << "Erro ao enviar lista de conexões para novo servidor backup." << endl;
								break; } } }
					bool fullListSent = connSocket->reply(new DMessage("fullListSent"));
					if(!fullListSent) cout << "Erro inesperado." << endl;
					//else cout << "Lista de conexões enviada com sucesso." << endl;
					} } }
	closeSocketAndFreePort(connSocket);
}

void DServer::listen()
{
	DMessage* request = NULL;
	while(true) {
		bool requestReceived = serverSocket->receive(&request);
		if(!requestReceived) continue;
		if(request->toString().substr(0,7) == "connect") {
			string clientName = request->toString().substr(8, request->toString().size());
			DClient* client = findClient(clientName);
			if(client == NULL) {
				DClient* newClient = new DClient(clientName, false);
				if(newClient->bad()) {
					cout << "DServer::listen() - Erro ao criar um novo cliente." << endl;
					serverSocket->reply(new DMessage("connection refused"));
					continue; }
				string clientPath = homeDir + "/dbox-server/" + clientName;
				bool clientPathCreated = (mkdir(clientPath.c_str(), 0777) == 0);
				if(clientPathCreated) {
					clients.push_back(newClient);
					acceptConnection(newClient, serverSocket->getSenderIp(), serverSocket->getSenderPort()); }
				else {
					cout << "DServer::listen() - Erro ao criar diretório para novo cliente." << endl;
					serverSocket->reply(new DMessage("connection refused"));
					continue; }	}
			else {
				if(client->isConnectedFrom(serverSocket->getSenderIp())) {
					cout << "Conexão recusada. Cliente já está conectado neste dispositivo." << endl;
					serverSocket->reply(new DMessage("connection refused"));
					continue; }
				else acceptConnection(client, serverSocket->getSenderIp(), serverSocket->getSenderPort()); } }
		else if(request->toString().substr(0,12) == "backupserver") {
			if(this->isPrimary) {
				DSocket* newConnectionSocket = new DSocket();
				bool socketOpen = newConnectionSocket->isOpen();
				if(!socketOpen) {
					cout << "DServer::listen() - Erro ao criar socket." << endl;
					continue; }
				int portNumber = getAvailablePort();
				if(portNumber == -1) {
					cout << "Erro. Número máximo de conexões excedido. Não há portas livres." << endl;
					continue; }
				ports[portNumber - SERVER_PORT - 1] = OCCUPIED;
				bool binded = newConnectionSocket->bindSocket("localhost", portNumber);
				if(!binded) {
					cout << "DServer::listen() - Erro ao criar socket. Problemas no bind()." << endl;
					continue; }
				newConnectionSocket->setDestination(inet_ntoa(serverSocket->getSenderIp()), ntohs(serverSocket->getSenderPort()));
				bool ackSent = serverSocket->reply(new DMessage("ack " + to_string(portNumber)));
				if(ackSent) {
					//cout << "Conexão estabelecida com novo backup server." << endl;
					thread* newConnection = new thread(&DServer::startBackup, this, newConnectionSocket);
					connections.push_back(newConnection);
					continue; }
				else cout << "DServer::listen() - Tentativa de conexão com backup server fracassou. Erro ao enviar ACK." << endl; }
			else serverSocket->reply(new DMessage("notprimary"));
		}
		else if(request->toString().substr(0,8) == "election") {
			//cout << "IM ALIVE: " << inet_ntoa(serverSocket->getSenderIp()) << ":" << ntohs(serverSocket->getSenderPort()) << "." << endl;  cout.flush();
			if(this->isPrimary) {
				serverSocket->setDestination(inet_ntoa(serverSocket->getSenderIp()), SERVER_PORT);
				serverSocket->send(new DMessage("newprimary " + to_string(this->serverId))); }
			serverSocket->reply(new DMessage("im alive"));
			if(!electionRunning && this->isPrimary == false) {
				this->electionRunning = true;
				startElection(); }
		}
		else if(request->toString().substr(0,10) == "updateback") {
			DSocket* newConnectionSocket = new DSocket();
			bool socketOpen = newConnectionSocket->isOpen();
			if(!socketOpen) {
				cout << "DServer::listen() - Erro ao criar socket." << endl;
				continue; }
			int portNumber = getAvailablePort();
			if(portNumber == -1) {
				cout << "Erro. Número máximo de conexões excedido. Não há portas livres." << endl;
				continue; }
			ports[portNumber - SERVER_PORT - 1] = OCCUPIED;
			bool binded = newConnectionSocket->bindSocket("localhost", portNumber);
			if(!binded) {
				cout << "DServer::listen() - Erro ao criar socket. Problemas no bind()." << endl;
				continue; }
			newConnectionSocket->setDestination(inet_ntoa(serverSocket->getSenderIp()), ntohs(serverSocket->getSenderPort()));
			bool ackSent = serverSocket->reply(new DMessage("ack " + to_string(portNumber)));
			if(ackSent) {
				thread* newConnection = new thread(&DServer::updateBackup, this, newConnectionSocket);
				connections.push_back(newConnection);
				continue; }
			else cout << "DServer::listen() - Tentativa de conexão com backup server fracassou. Erro ao enviar ACK." << endl;
		}
		else if(request->toString().substr(0,10) == "newprimary") {
			unsigned int primaryId = atoi(request->toString().substr(11).c_str());
			list<ReplicaManager*>::iterator it;
			for(it = replicaManagers.begin(); it != replicaManagers.end(); it++) {
				if((*it)->id == primaryId) {
					cout << "Novo primário. ID: " << primaryId << ". IP: " << string(inet_ntoa((*it)->address)) << "." << endl; cout.flush();	
					(*it)->primary == true;
					this->setPrimary(string(inet_ntoa((*it)->address)));
					serverSocket->reply(new DMessage("im aware")); } }
		}
		else if(request->toString().substr(0,10) == "removeserv") {
			unsigned int removeId = atoi(request->toString().substr(11).c_str());
			list<ReplicaManager*>::iterator it;
			bool found = false;
			for(it = replicaManagers.begin(); it != replicaManagers.end(); it++)
				if((*it)->id == removeId) { found = true; break; }
			if(found) replicaManagers.erase(it);
		} }
}

void DServer::acceptConnection(DClient* client, struct in_addr clientIp, unsigned short clientPort)
{
	int attempts = 0;
	while(attempts < 3) {
		attempts++;
		int portNumber = getAvailablePort();
		if(portNumber != -1) {
			DSocket* newConnectionSocket = new DSocket();
			bool connSocketOpen = newConnectionSocket->isOpen();
			if(!connSocketOpen) {
				cout << "DServer::acceptConnection() - Tentativa " << attempts << " de conexão com o cliente fracassou. Socket fechado." << endl;
				continue; }
			bool binded = newConnectionSocket->bindSocket("localhost", portNumber);
			if(!binded) {
				cout << "DServer::acceptConnection() - Tentativa " << attempts << " de conexão com o cliente fracassou. Problemas no bind()." << endl;
				ports[portNumber - SERVER_PORT - 1] = UNAVAILABLE;
				continue; }
			ports[portNumber - SERVER_PORT - 1] = OCCUPIED;
			newConnectionSocket->setDestination(inet_ntoa(clientIp),(int)clientPort);
			client->getConnectionsList()->push_back(newConnectionSocket);
			thread* newConnection = new thread(&DServer::messageProcessing, this, client, newConnectionSocket);
			clisocks.push_back(newConnectionSocket);
			connections.push_back(newConnection);
			bool ackSent = serverSocket->reply(new DMessage("ack " + to_string(portNumber)));
			if(ackSent) {
				//cout << "Conexão aceita. Cliente: " << client->getName() << ". IP: " << inet_ntoa(clientIp) << ". Porta: " << clientPort << "." << endl;
				break; }
			else cout << "DServer::acceptConnection() - Tentativa " << attempts << " de conexão com o cliente fracassou. Erro ao enviar ACK." << endl; }
		else {
			cout << "Impossível criar conexão com cliente. Servidor no limite de conexões simultâneas." << endl;
			serverSocket->reply(new DMessage("connection refused"));
			break; } }
	if(attempts == 3) cout << "DServer::acceptConnection() - Todas as tentativas de conexão com o cliente fracassaram." << endl;
}

int DServer::getAvailablePort()
{
	for(int i = 0; i < MAX_CONNECTIONS; i++)
		if(ports[i] == AVAILABLE) return SERVER_PORT + 1 + i;
	return -1;
}

void DServer::sendFile(DClient* client, DSocket* connection, DMessage* message)
{
	string fileName = message->toString().substr(5);
	string filePath = homeDir + "/dbox-server/" + client->getName() + "/" + fileName;
	ifstream file;
	file.open(filePath, ifstream::in | ifstream::binary);
	if(!file.is_open()) {
		DMessage* notFound = new DMessage("file not found");
		connection->reply(notFound);
		return; }
	file.seekg(0, file.end);
	int fileSize = file.tellg();
	file.seekg(0, file.beg);
	struct stat fstat;
	bool statSuccess = (stat(filePath.c_str(),&fstat) == 0);
	if(!statSuccess) {
		cout << "DServer::sendFile() - Erro. Não foi possível obter informações sobre a data de modificação do arquivo." << endl;
		return; }
	DMessage* prepareToReceive = new DMessage("send ack " + to_string(fileSize));
	bool replySent = connection->reply(prepareToReceive);
	if(!replySent) {
		cout << "DServer::sendFile() - Erro ao enviar confirmação para envio de arquivo." << endl;
		return; }
	DMessage* clientResponse = NULL;
	bool clientResponseReceived = connection->receive(&clientResponse);
	if(!clientResponseReceived) {
		cout << "DServer::sendFile() - Erro. Resposta do cliente à confirmação para envio não recebida." << endl;
		return; }
	if(clientResponse->toString() != "send ack ack") {
		cout << "DServer::sendFile() - Erro. Cliente recusou recebimento do arquivo." << endl;
		return; }
	int totalPackets = (int)((fileSize/BUFFER_SIZE)+1);
	int lastPacketSize = fileSize%BUFFER_SIZE;
	int packetsSent = 0;
	int filePointer = 0;
	int blockSize = BUFFER_SIZE;
	char* packetContent = new char[BUFFER_SIZE];
	while(packetsSent < totalPackets) {
		packetsSent++;
		file.seekg(filePointer, file.beg);
		filePointer += BUFFER_SIZE;
		if(packetsSent == totalPackets) blockSize = lastPacketSize;
		file.read(packetContent, blockSize);					
		DMessage* packet = new DMessage(packetContent,blockSize);
		bool packetSent = connection->reply(packet);
		if(!packetSent) {
			cout << "DServer::sendFile() - Erro ao enviar pacote para o servidor. Envio interrompido." << endl;
			return; }
		DMessage* packetDeliveryStatus = NULL;
		bool packetDeliveryResponseReceived = connection->receive(&packetDeliveryStatus);
		if(!packetDeliveryResponseReceived) {
			cout << "DServer::sendFile() - Erro. Confirmação de entrega de pacote não recebida. Envio interrompido." << endl;
			return; }
		if(packetDeliveryStatus->toString() == "packet received") continue;
		else {
			cout << "DServer::sendFile() - Erro. Status de entrega de pacote desconhecido." << endl;
			return; } }
	file.close();
	DMessage* lastModificationTime = new DMessage(to_string(fstat.st_mtim.tv_sec));
	bool lmtSent = connection->reply(lastModificationTime);
	if(!lmtSent) {
		cout << "DServer::sendFile() - Erro ao informar o cliente sobre a data da última modificação do arquivo." << endl;
		return; }
	//cout << "Envio de arquivo completo. Cliente: " << client->getName() << ". IP: " << inet_ntoa(connection->getDestinationIp()) << ". Arquivo: " << fileName << "." << endl;
}

void DServer::deleteFile(DClient* client, DSocket* connection, DMessage* message)
{
	string fileName = message->toString().substr(7);
	string filePath = homeDir + "/dbox-server/" + client->getName() + "/" + fileName;
	struct stat fstat;
	bool statSuccess = (stat(filePath.c_str(),&fstat) == 0);
	if(!statSuccess) {
		DMessage* notFound = new DMessage("file not found");
		connection->reply(notFound);
		return; }
	bool fileRemoved = (remove(filePath.c_str()) == 0);
	if(!fileRemoved) {
		cout << "DServer::deleteFile() - Erro ao excluir arquivo." << endl;
		DMessage* error = new DMessage("error");
		connection->reply(error);
		return; }
	DMessage* removed = new DMessage("removed");
	connection->reply(removed);
	client->getFilesList()->clear();
	client->fillFilesList("/dbox-server");
	//cout << "Arquivo deletado do servidor: " << fileName << ". Cliente: " << client->getName() << "." << endl;
}

void DServer::receiveFile(DClient* client, DSocket* connection, DMessage* message)
{
	int fileSize = atoi(message->toString().substr(message->toString().find_last_of(" ")+1).c_str());
	string fileName = message->toString().substr(8, message->toString().find_last_of(" ")-8);
	string filePath = homeDir + "/dbox-server/" + client->getName() + "/" + fileName;
	ofstream newFile;
	newFile.open(filePath, ofstream::trunc | ofstream::binary);
	if(!newFile.is_open()) {
		cout << "DServer::receiveFile() - Erro ao receber arquivo. Não foi possível criar cópia local." << endl;
		return; }
	DMessage* serverReply = new DMessage("receive ack");
	bool serverReplySent = connection->reply(serverReply);
	if(!serverReplySent) {
		cout << "DServer::receiveFile() - Erro ao enviar confirmação para recebimento de arquivo." << endl;
		return; }
	int totalPackets = (int)((fileSize/BUFFER_SIZE)+1);
	int packetsReceived = 0;
	int packetSize = 0;
	DMessage* packet = NULL;
	while(packetsReceived < totalPackets) {
		packetsReceived++;				
		bool packetReceived = connection->receive(&packet);
		if(!packetReceived) {
			cout << "DServer::receiveFile() - Erro ao receber pacote do arquivo. Recebimento interrompido." << endl;
			newFile.close();
			return; }
		DMessage* confirmation = new DMessage("packet received");
		bool confirmationSent = connection->reply(confirmation);
		if(!confirmationSent) {
			cout << "DServer::receiveFile() - Erro ao enviar confirmação de recebimento de pacote de dados. Recebimento interrompido." << endl;
			newFile.close();
			return; }
		newFile.write(packet->get(),packet->length()); }
	newFile.close();
	DMessage* lastModificationTime = NULL;
	bool lmtReceived = connection->receive(&lastModificationTime);
	if(!lmtReceived) {
		cout << "DServer::receiveFile() - Erro. Data da última modificação do arquivo não recebida." << endl;
		return; }
	time_t lmt = atol(lastModificationTime->toString().c_str());
	struct utimbuf utb;
	utb.actime = lmt;
	utb.modtime = lmt;
	bool modTimeChanged = (utime(filePath.c_str(),&utb) == 0);
	if(!modTimeChanged) {
		cout << "DServer::receiveFile() - Erro. Não foi possível modificar a data da última modificação e acesso." << endl;
		return; }
	//cout << "Arquivo recebido com sucesso. Cliente: " << client->getName() << ". Arquivo: " << fileName << "." << endl;
	client->updateFilesList(fileName, "/dbox-server/" + client->getName());	
	//client->getFilesList()->clear();
	//client->fillFilesList("/dbox-server");
}

void DServer::listFiles(DClient* client, DSocket* connection, DMessage* message)
{
	int totalFiles = client->getFilesList()->size();	
	if(totalFiles == 0) {
		DMessage* emptyList = new DMessage("file list empty");
		connection->reply(emptyList);
		return; }
	DMessage* listSize = new DMessage("list size " + to_string(totalFiles));
	bool replySent = connection->reply(listSize);
	if(!replySent) return;
	DMessage* clientReply = NULL;
	bool clientReplyReceived = connection->receive(&clientReply);
	if(!clientReplyReceived) return;
	if(clientReply->toString() != "confirm") return;
	list<DFile*>::iterator it;
	list<DFile*>* clientFiles = client->getFilesList();
	for(it = clientFiles->begin(); it != clientFiles->end(); it++) {
		DFile* clientFile = *(it);
		DMessage* fileInfo = new DMessage(clientFile->getName() + " [" + to_string(clientFile->getSize()) + "," + to_string(clientFile->getLastModified()) + "]");
		bool fileInfoSent = connection->reply(fileInfo);
		if(!fileInfoSent) return;
		DMessage* fileInfoSentReply = NULL;
		bool fileInfoReceived = connection->receive(&fileInfoSentReply);
		if(!fileInfoReceived) return;
		if(fileInfoSentReply->toString() != "confirm") return;
	}
	if(string(message->get()).size() > 4) closeSocketAndFreePort(connection);
}

void DServer::closeSocketAndFreePort(DSocket* connSocket)
{
	if(connSocket->closeSocket())
		ports[ntohs(connSocket->getSocketPort()) - SERVER_PORT - 1] = AVAILABLE;
	else
		cout << "DServer::closeSocketAndFreePort() - Erro ao fechar socket. Porta continuará ocupada." << endl;
}
void DServer::listClientFiles(string clientName)
{
	DClient* client = findClient(clientName);
	if(client == NULL) {
		cout << "DServer::listClientFiles() - Erro. Cliente não encontrado." << endl;
		return; }
	cout << endl << "\e[1mArquivos do cliente " << clientName << "\e[0m" << endl << endl;
	client->listFiles();
	cout << endl;
}

bool DServer::closeConnection(DClient* client, DSocket* connection)
{
	clisocks.remove(connection);
	client->getConnectionsList()->remove(connection);
	if(client->isConnectedFrom(connection->getDestinationIp())) cout << "Não removido." << endl;
	ports[ntohs(connection->getSocketPort())-SERVER_PORT-1] = AVAILABLE;
	DMessage* closeConfirm = new DMessage("connection closed");
	bool replySent = connection->reply(closeConfirm);
	if(replySent) {
		//cout << "Conexão terminada. Cliente: " << client->getName() << ". Dispositivo: " << inet_ntoa(connection->getSenderIp()) << endl;
		connection->closeSocket();
		return true; }
	return false;
}

void DServer::messageProcessing(DClient* client, DSocket* connection)
{
	while(true) {
		DMessage* message = NULL;
		bool messageReceived = connection->receive(&message);
		if(messageReceived) {
			if(message->toString() == "close connection") {
				bool connectionClosed = closeConnection(client,connection);
				if(connectionClosed) break;	}	 
			if(message->toString().substr(0,7) == "receive") {
				client->getMutex()->lock();
				receiveFile(client, connection, message);
				client->getMutex()->unlock(); }
			if(message->toString().substr(0,6) == "delete") {
				client->getMutex()->lock();
				deleteFile(client, connection, message);
				client->getMutex()->unlock(); }
			if(message->toString().substr(0,4) == "send") {
				client->getMutex()->lock();
				sendFile(client, connection, message);
				client->getMutex()->unlock(); }
			if(message->toString() == "list") {
				client->getMutex()->lock();
				listFiles(client, connection, message);
				client->getMutex()->unlock(); } }
		else continue; }
}

bool DServer::notifyPrimary()
{
	replicaManagers.clear(); 	// ao instanciar um DServer, ele se autoinsere como primário ao criar o objeto.
								// chamar clear() desfaz essa inserção
	DMessage* message = new DMessage("backupserver");
	serverSocket->setDestination(primaryIp.c_str(),SERVER_PORT);
	bool messageSent = serverSocket->send(message);
	if(messageSent) {
		DMessage* response = NULL;
		bool replyReceived = serverSocket->receive(&response);
		if(replyReceived) {
			if(string(response->get()).substr(0,3) != "ack") {
				cout << "Resposta do servidor: " << response->get() << endl;
				return false; }
			int connPort = atoi(string(response->get()).substr(4).c_str());
			serverSocket->setDestination(primaryIp.c_str(),connPort);
			bool upsent = serverSocket->send(new DMessage("updatestart"));
			replyReceived = serverSocket->receive(&response);
			if(!replyReceived || !upsent || string(response->get()) == "startfail") {
				cout << "DServer::notifyPrimary() - Erro. Backack não recebido ou servidor falhou." << endl;
				return false; }
			unsigned int thisId = strtoul(string(response->get()).substr(8).c_str(),NULL,0);
			this->serverId = thisId;
			bool serversListRequested = serverSocket->send(new DMessage("sendserverlist"));
			if(!serversListRequested) return false;
			else {
				DMessage* listItem = NULL;				
				while(serverSocket->receive(&listItem)) {
					if(string(listItem->get()) == "fullListSent") break;
					string ip = string(listItem->get()).substr(3,string(listItem->get()).substr(3).find_first_of(",",0));
					struct ReplicaManager* rm = new struct ReplicaManager;
					rm->primary = false;
					if(ip == "primary") {
						rm->primary = true;
						ip = primaryIp; }
					inet_aton(ip.c_str(),&(rm->address));
					rm->id = atoi(string(listItem->get()).substr(string(listItem->get()).find_first_of(",",0)+1).c_str());
					replicaManagers.push_back(rm);
					serverSocket->send(new DMessage("ok"));	} }
			bool clientsListRequested = serverSocket->send(new DMessage("sendclientlist"));
			if(!clientsListRequested) return false;
			else {
				DMessage* listItem = NULL;				
				while(serverSocket->receive(&listItem)) {
					if(string(listItem->get()) == "fullListSent") break;
					string clientName = string(listItem->get()).substr(string(listItem->get()).find_first_of(":",0)+1);
					bool clientFound = false;
					list<DClient*>::iterator it;
					for(it = clients.begin(); it != clients.end(); ++it) {
						DClient* cl = *(it);
						if(cl->getName() == clientName) {
							clientFound = true;
							break; } }
					if(!clientFound) {
						//cout << "Cliente não encontrado localmente: " << clientName << "." << endl;
						clients.push_back(new DClient(clientName, false)); }
					//else {
						//cout << "Cliente encontrado: " << clientName << endl; }
					serverSocket->send(new DMessage("ok"));	} }
			bool connListRequested = serverSocket->send(new DMessage("sendconnlist"));
			if(!connListRequested) return false;
			else {
				DMessage* listItem = NULL;		
				tempsocks.clear();		
				while(serverSocket->receive(&listItem)) {
					if(string(listItem->get()) == "fullListSent") break;
					string ip = string(listItem->get()).substr(0,string(listItem->get()).find_first_of(":",0));
					int cliport = atoi(string(listItem->get()).substr(string(listItem->get()).find_first_of(":",0)+1).c_str());
					DSocket* cs = new DSocket();
					//cout << "Conexão ativa: " << ip << ":" << cliport << "." << endl;
					cs->setDestination(ip.c_str(),cliport);
					tempsocks.push_back(cs);
					serverSocket->send(new DMessage("ok"));	} }
			syncWithPrimary(DONT_PRINT);
			list<ReplicaManager*>::iterator it, it2;
			for(it = replicaManagers.begin(); it != replicaManagers.end(); ++it)
				if((*it)->id == thisId) break;
			string myIp = string(inet_ntoa((*it)->address));
			it2 = it;
			bool found = false;
			for(it = replicaManagers.begin(); it != replicaManagers.end(); ++it)
				if((*it)->id != thisId && string(inet_ntoa((*it)->address)) == myIp) {
					found = true;
					serverSocket->setDestination(primaryIp.c_str(),SERVER_PORT);
					serverSocket->send(new DMessage("removeserv " + to_string(thisId)));
					this->serverId = (*it)->id; }
			if(found) replicaManagers.erase(it2);
			for(it = replicaManagers.begin(); it != replicaManagers.end(); ++it)
				if((*it)->primary == true) break;			
			if((*it)->id < this->serverId) {
				this->electionRunning = true;
				startElection(); }
			return true; }
		else return false; }
}

bool DServer::updatePrimary()
{
	string primaryWhenUpdateBegin = primaryIp;
	int avport = getAvailablePort();
	ports[avport - SERVER_PORT - 1] = OCCUPIED;
	DSocket* updateSocket = new DSocket();
	bool binded = updateSocket->bindSocket("localhost", avport);
	if(!binded) {
		cout << "Erro no bind em updatePrimary." << endl;
		return false; }
	DMessage* message = new DMessage("updateback");
	updateSocket->setDestination(primaryIp.c_str(),SERVER_PORT);
	bool messageSent = updateSocket->send(message);
	if(messageSent) {
		DMessage* response = NULL;
		bool replyReceived = updateSocket->receive(&response);
		if(replyReceived) {
			if(string(response->get()).substr(0,3) != "ack") {
				cout << "Conexão recusada pelo servidor primário." << endl;
				closeSocketAndFreePort(updateSocket);
				return false; }
			int connPort = atoi(string(response->get()).substr(4).c_str());
			updateSocket->setDestination(primaryIp.c_str(),connPort);
			bool upsent = updateSocket->send(new DMessage("updatestart"));
			if(!upsent) cout << "Updatestart not sent" << endl;
			replyReceived = updateSocket->receive(&response);
			if(!replyReceived || !upsent || string(response->get()) == "startfail") {
				cout << "DServer::notifyPrimary() - Erro. Backack não recebido ou servidor falhou." << endl;
				closeSocketAndFreePort(updateSocket);
				return false; }
			bool serversListRequested = updateSocket->send(new DMessage("sendserverlist"));
			if(!serversListRequested) {
				closeSocketAndFreePort(updateSocket);
				return false; }
			else {
				replicaManagers.clear();
				DMessage* listItem = NULL;				
				while(updateSocket->receive(&listItem)) {
					if(string(listItem->get()) == "fullListSent") break;
					string ip = string(listItem->get()).substr(3,string(listItem->get()).substr(3).find_first_of(",",0));
					struct ReplicaManager* rm = new struct ReplicaManager;
					rm->primary = false;
					if(ip == "primary") {
						rm->primary = true;
						ip = primaryIp; }
					inet_aton(ip.c_str(),&(rm->address));
					rm->id = atoi(string(listItem->get()).substr(string(listItem->get()).find_first_of(",",0)+1).c_str());
					replicaManagers.push_back(rm);
					updateSocket->send(new DMessage("ok"));	} }
			bool clientsListRequested = updateSocket->send(new DMessage("sendclientlist"));
			if(!clientsListRequested) {
				closeSocketAndFreePort(updateSocket);
				return false; }
			else {
				DMessage* listItem = NULL;
				while(updateSocket->receive(&listItem)) {
					if(string(listItem->get()) == "fullListSent") break;
					string clientName = string(listItem->get()).substr(string(listItem->get()).find_first_of(":",0)+1);
					bool clientFound = false;
					list<DClient*>::iterator it;
					for(it = clients.begin(); it != clients.end(); ++it) {
						DClient* cl = *(it);
						if(cl->getName() == clientName) {
							clientFound = true;
							break; } }
					if(!clientFound) {
						//cout << "Cliente não encontrado localmente: " << clientName << "." << endl;
						clients.push_back(new DClient(clientName, false)); }
					updateSocket->send(new DMessage("ok"));	} }
			bool connListRequested = updateSocket->send(new DMessage("sendconnlist"));
			if(!connListRequested) return false;
			else {
				DMessage* listItem = NULL;		
				tempsocks.clear();		
				while(updateSocket->receive(&listItem)) {
					if(string(listItem->get()) == "fullListSent") break;
					string ip = string(listItem->get()).substr(0,string(listItem->get()).find_first_of(":",0));
					int cliport = atoi(string(listItem->get()).substr(string(listItem->get()).find_first_of(":",0)+1).c_str());
					DSocket* cs = new DSocket();
					//cout << "Conexão ativa: " << ip << ":" << cliport << "." << endl;
					cs->setDestination(ip.c_str(),cliport);
					tempsocks.push_back(cs);
					updateSocket->send(new DMessage("ok"));	} }
			syncWithPrimary(DONT_PRINT);
			closeSocketAndFreePort(updateSocket);
			return true; }
		else {
			if(primaryWhenUpdateBegin == primaryIp && this->isPrimary == false) {
				cout << "SERVIDOR PRIMÁRIO FALHOU." << endl; cout.flush();
				if(!electionRunning) {
					this->electionRunning = true;
					startElection(); }
				closeSocketAndFreePort(updateSocket);
				return false; } } }
	closeSocketAndFreePort(updateSocket);
}

void DServer::auditElection(DSocket* socket, int* voters)
{
	socket->setTimeOut(2);
	socket->send(new DMessage("election"));
	//cout << "Election enviado para " << inet_ntoa(socket->getDestinationIp()) << ":" << ntohs(socket->getDestinationPort()) << endl; cout.flush();	
	DMessage* reply = NULL;
	bool replyReceived = socket->receive(&reply);
	if(replyReceived && string(reply->get()) == "im alive") {		
		//cout << "Im alive recebido de: " << inet_ntoa(socket->getSenderIp()) << ":" << ntohs(socket->getSenderPort()) << endl; cout.flush();
		*voters = *voters - 1; }
	//else {
		//cout << "RM Dead: " << inet_ntoa(socket->getDestinationIp()) << ":" << ntohs(socket->getDestinationPort()) << endl; cout.flush(); }
	closeSocketAndFreePort(socket);
}

void DServer::sendRepWarning(DSocket* socket)
{
	socket->send(new DMessage("newprimary " + to_string(this->serverId)));
	//DMessage* reply = NULL;
	//bool replyReceived = socket->receive(&reply);
	//if(replyReceived && string(reply->get()) == "im aware") {		
		//cout << "RM em " << inet_ntoa(socket->getDestinationIp()) << " está ciente." << endl; cout.flush(); }
	//else {
		//cout << "RM em " << inet_ntoa(socket->getDestinationIp()) << " NÃO está ciente." << endl; cout.flush(); }
	closeSocketAndFreePort(socket);
}

void DServer::startElection()
{
	cout << "Nova eleição iniciada..." << endl; cout.flush();
	list<thread*> votes;
	int* voters = new int(0);
	list<ReplicaManager*>::iterator it;
	//cout << "Número de potenciais votantes: " << replicaManagers.size() << endl; cout.flush();	
	for(it = replicaManagers.begin(); it != replicaManagers.end(); ++it) {
		//cout << (*it)->id << " > " << this->serverId << endl;
		if((*it)->id > this->serverId) *voters = *voters + 1; }
	//cout << "Número de votantes auferido: " << *voters << endl; cout.flush();	
	int savedVoters = *voters;
	for(it = replicaManagers.begin(); it != replicaManagers.end(); ++it) {
		if((*it)->id > this->serverId) {
			DSocket* newSock = new DSocket();
			int portNumber = getAvailablePort();
			newSock->bindSocket("localhost", portNumber);
			ports[portNumber - SERVER_PORT - 1] = OCCUPIED;
			newSock->setDestination(inet_ntoa((*it)->address), SERVER_PORT);
			std::thread* newVote = new thread(&DServer::auditElection, this, newSock, voters); 
			votes.push_back(newVote);
		}
	}
	list<thread*>::iterator it2;
	for(it2 = votes.begin(); it2 != votes.end(); ++it2)
		(*it2)->join();
	//cout << "Resultado da votação: " << *voters << "." << endl; cout.flush();	
	this->electionRunning = false;
	if(*voters == savedVoters) {
		cout << "Eleição vencida. Este servidor é o novo primário." << endl; cout.flush();	
		this->isPrimary = true;
		list<ReplicaManager*>::iterator it;
		for(it = replicaManagers.begin(); it != replicaManagers.end(); it++)
			if((*it)->primary == true) (*it)->primary = false;
		for(it = replicaManagers.begin(); it != replicaManagers.end(); it++)
			if((*it)->id == this->serverId) (*it)->primary = true;
		for(it = replicaManagers.begin(); it != replicaManagers.end(); ++it) {
			if((*it)->id != this->serverId) {
				DSocket* newSock = new DSocket();
				int portNumber = getAvailablePort();
				newSock->bindSocket("localhost", portNumber);
				ports[portNumber - SERVER_PORT - 1] = OCCUPIED;
				newSock->setDestination(inet_ntoa((*it)->address), SERVER_PORT);
				std::thread* newWarning = new thread(&DServer::sendRepWarning, this, newSock); }
		}
		list<DSocket*>::iterator it2;
		for(it2 = tempsocks.begin(); it2 != tempsocks.end(); ++it2) {
			cout << "Avisando cliente no IP " << inet_ntoa((*it2)->getDestinationIp()) << " da mudança de primary." << endl;
			serverSocket->setDestination(inet_ntoa((*it2)->getDestinationIp()), CLIENT_UPDATE_PORT);
			serverSocket->send(new DMessage("newprimary")); }
	} else {
		cout << "Eleição perdida." << endl; cout.flush();	
	}
}

void DServer::setPrimary(string _primaryIp)
{
	primaryIp = _primaryIp;
	isPrimary = false;
	cout << "Sincronização com o servidor primário ativada." << endl;
}

void DServer::syncDaemon()
{
	while(true) {
		this_thread::sleep_for(chrono::seconds(10));
		if(this->isPrimary) {
			//cout << "Sync daemon desativado." << endl; cout.flush();
			continue; }
		updatePrimary();
	}
}

void DServer::syncWithPrimary(PrintOption option)
{
	list<DClient*>::iterator it;
	list<DFile*> clientFiles;
	for(it = clients.begin(); it != clients.end(); ++it) {
		DClient* client = *(it);
		DClient* simmClient = new DClient(client->getName(), false);
		simmClient->fillFilesList("/dbox-server");
		bool clientConnected = simmClient->connect(primaryIp.c_str(), SERVER_PORT);
		if(!clientConnected) {
			cout << "DClient::connect() - Erro ao conectar cliente." << endl;
			return; }
		bool serverFilesListCreated = simmClient->listServerFiles(DONT_PRINT);
		if(!serverFilesListCreated) {
			cout << "DClient::listServerFiles() - Erro ao obter lista de arquivos do cliente no servidor." << endl;
			return; }
		simmClient->synchronize("/dbox-server", DONT_PRINT);
		simmClient->updateReverse("/dbox-server");
		simmClient->closeConnection();
		//client->getFilesList()->clear();
		//client->fillFilesList("/dbox-server");
		if(option == PRINT) cout << "Cliente '" << client->getName() << "': sincronização OK." << endl;	}
} 

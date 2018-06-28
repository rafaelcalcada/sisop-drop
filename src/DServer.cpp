#include "../include/DServer.h"

DServer::DServer(string confFile)
{
	serverSocket = new DSocket();
	if(serverSocket->isOpen()) {
		bool bindOk = serverSocket->bindSocket("localhost",SERVER_PORT);
		if(bindOk) {
			for(int i = 0; i < MAX_CONNECTIONS; i++) ports[i] = AVAILABLE;
			isWorking = this->initialize() && this->loadConfFile(); && this->initializeRing() }
		else {
			cout << "DServer::DServer() - Erro. Chamada de serverSocket->bind() retornou código de erro." << endl;
			serverSocket->closeSocket();
			isWorking = false; } }
	else {
		cout << "DServer::DServer() - Erro. Chamada de serverSocket->isOpen() retornou código de erro." << endl;
		isWorking = false; }
}

bool DServer::initialize()
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
				DClient* newClient = new DClient(clientName);
				clients.push_back(newClient);
				newClient->fillFilesList("/dbox-server/" + clientName); } } 
			return true; }
	else if(mkdir(serverPath.c_str(), 0777) != 0) {
		cout << "DServer::initialize() - Erro. Não conseguiu criar diretório raiz do servidor." << endl;
		return false; }
	else
		return true;
}

bool DServer::loadConfFile(string confFile)
{
	cout << "DServer::loadConfFile() - Lendo endereços de \"" << confFile << "\"." << endl;
	thisServerAddresses = getIpAddressList();
	ifstream infile;
	try {
		infile.open(confFile); }
	catch (const exception &e) {
		cout << "DServer::loadConfFile() - Exception. Lançado " << e.what() << " ao abrir o arquivo de configuração para leitura." << endl;
		return false; }

	string confLine;
	int numServers = 0;
	string ip;
	int port;
	thisServerID = -1;
	while (!infile.eof()) {
		infile >> confLine;
		if (confLine != "") {
			int colon = confLine.find(':');
			try {
				ip = confLine.substr(0, colon);
				port = stoi(confLine.substr(colon+1)); }
			catch (const exception &e) {
				cout << "DServer::loadConfFile() - Exception. Lançado " << e.what() << " processando endereços de servidor." << endl;
				return false; }
			if (!validateAddress(ip, port))
				cout << "DServer::loadConfFile() - Erro. \"" << confLine << "\" não é um endereço ip:porta válido." << endl;
			if (thisServerAddresses.find(ip) != string::npos)
				thisServerID = numServers;
			pair<string, int> newServer = make_pair(ip, port);
			servers.push_back(newServer);

			numServers++;
			cout << "DServer::loadConfFile() - Servidor #" << numServers << "  " << ip << ":" << port;
			if (thisServerID == numServers - 1)
				cout << " (este servidor)";
			cout << endl; } }
	if (thisServerID == -1) {
		cout << "DServer::loadConfFile() - Erro. Arquivo de configuração não tem entrada para este servidor." << endl; 
		return false; }
	if (numServers == 0) {
		cout << "DServer::loadConfFile() - Erro. Nenhum servidor adicionado." << endl; 
		return false; }
	else if (numServers == 1)
		cout << "DServer::loadConfFile() - Aviso. Apenas um servidor encontrado. Recuperação em caso de falhas não será possível." << endl;

	serverStatus = OFFLINE;
	return true;
}

string DServer::getIpAddressList() {
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	char *addr;
	string r = "|";

	getifaddrs (&ifap);
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family==AF_INET) {
			sa = (struct sockaddr_in *) ifa->ifa_addr;
			addr = inet_ntoa(sa->sin_addr);
			if (addr != "127.0.0.1")
				r += addr + "|"; } }

	freeifaddrs(ifap);
	return r;
}

bool DServer::initializeRing()
{
	upRingSocket = new DSocket(TCP);
	if(!upRingSocket->bindSocket("localhost",SERVER_PORT-1)) {
		cout << "DServer::setUpRing() - Erro. Chamada de upRingSocket->bind() retornou código de erro." << endl;
		upRingSocket->closeSocket();
		return false; }
	if(!upRingSocket->listen()) {
		cout << "DServer::setUpRing() - Erro. Chamada de upRingSocket->listen() retornou código de erro." << endl;
		upRingSocket->closeSocket();
		return false; }
	upRingConnectedSocket = NULL;
	DownRingSocket = new DSocket(TCP);
	isUpRingConnected = false;
	isDownRingConnected = false;
	filePartsRemaining = 0;
	return true:
}

DClient* DServer::findClient(string clientName)
{
	list<DClient*>::iterator it;
	for(it = clients.begin(); it != clients.end(); ++it) {
		DClient* client = *(it);
		if(client->getName() == clientName) return client; }
	return NULL;
}

void DServer::coordinate()
{
	while(true) {
		if (!isRingConnected)
			if (!connectRing())
				cout << "DServer::coordinate() - Erro. Não foi possível estabelecer conexão em anel." << endl;
		if (serverStatus == MASTER) {
			if (ringMessage) {
				DMessage* validationMessage;
				if (upRingConnectedSocket->receive(&validationMessage)) {
					if (validationMessage == ringMessage)
						delete RingMessage;
						ringMessage = NULL;
					else if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
						cout << "DServer::coordinate() - Erro. Erro ao checar a integridade da mensagem." << endl;
						isUpRingConnected = false; }
					delete validationMessage; } }
			if (!rinqQueue.empty()) {
				if (downRingSocket->send(ringQueue.top())) {
					delete ringQueue.top();
					ringQueue.pop(); }
				else if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
					cout << "DServer::coordinate() - Erro. downRingSocket->send() retornou código de erro. Envio será tentado novamente se a conexão estiver aberta." << endl;
					isDownRingConnected = false; } } }
		else if (serverStatus == SLAVE) {
			if (!ringMessage)
				if (upRingConnectedSocket->receive(&ringMessage))
					if (!processUpdate()) {
						cout << "DServer::coordinate() - Erro. Não foi possível processar a atualização recebida." << endl;
						breakRing(); }
				else if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
						cout << "DServer::coordinate() - Erro. upRingSocket->receive() retornou código de erro. Recebimento será tentado novamente se a conexão estiver aberta." << endl;
						isUpRingConnected = false; }
			if (ringMessage)
				if (downRingSocket->send(ringMessage))
						delete ringMessage;
						ringMessage = NULL;
				else if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
					cout << "DServer::coordinate() - Erro. Chamada de downRingSocket->send() retornou código de erro. Envio será tentado novamente se a conexão estiver aberta." << endl;
					isDownRingConnected = false; } }
		else if (serverStatus == OFFLINE) {
			cout << "DServer::coordinate() - Servidor está offline." << endl;
			breakRing(); } } }

bool DServer::connectRing()
{
	if (!isUpRingConnected) {
		if (upRingConnectedSocket)
			delete upRingConnectedSocket;
		if (upRingConnectedSocket = upRingSocket->acceptConnection())
			isUpRingConnected = true;
		if (getSocketServerID(upRingConnectedSocket) > thisServerID)
			serverStatus = MASTER;
		else
			serverStatus = SLAVE; }

	if (!isDownRingConnected) {
		int i = thisServerID + 1;
		while ((!isDownRingConnected) && (i != thisServerID)) {
			for (int j = 0; j < MAX_RETRIES; j++) {
				cout << "DServer::connectRing() - Conectando ao servidor #" << (i + 1) << " - "
					 << get<POS_IP>(servers[i])  << ":"
					 << get<POS_PORT>(servers[i])  << endl;
				downRingSocket->setDestination(get<POS_IP>servers[i], get<POS_PORT>(servers[i]));
				if (downRingSocket->connectSocket())
					isDownRingConnected = true; }
			id += 1 % servers.size(); }
		if (!isDownRingConnected) {
			cout << "DServer::connectRing() - Nenhum servidor aceitou conexão." << endl;
			serverStatus = OFFLINE; } }
}

int DServer::getSocketServerID(DSocket* socket)
{
	string thisIp = socket.getDestinationIp;
	int thisPort = socket.getDestinationPort;
	for (int i = 0; i < servers.size(); i++)
		if (thisIp == get<POS_IP>(servers[i])) && (thisPort == get<POS_PORT>(servers[i]))
			return i;
	return -1;
}

bool DServer::processUpdate()
{
	ofstream receivingFile;
	if (filePartsRemaining > 0) {
		receivingFile.open(receivingFilePath, ofstream::app | ofstream::binary);
		receivingFile.write(ringMessage->get(),ringMessage->length());
		receivingFile.close();
		filePartsRemaining--; }
	else if (ringMessage.toString().substr(0,12) == "receive file") {
		receivingFilePath = ringMessage.toString;
		receivingFilePath = receivingFilePath.substr(receivingFilePath.find(" ")+1);
		string fileParts = fileName;
		receivingFilePath = receivingFilePath.substr(receivingFilePath.find(" ")+1);
		fileParts = fileParts.substr(0, fileParts.find(" "));
		receivingFilePartsRemaining = stoi(fileParts);
		receivingFile.open(receivingFilePath, ofstream::trunc | ofstream::binary);
		receivingFile.close(); }
	else if (ringMessage.toString().substr(0,11) == "delete file") {
		string filePath = ringMessage.toString;
		filePath = filePath.substr(fileName.find(" ")+1);
		remove(filePath.c_str()) }
	else {
		cout << "DServer::processUpdate() - Erro. Comando \"" << ringMessage.toString.substr(0,20);
		if (ringMessage.toString.size() > 20)
			cout << "...";
		cout << "\" não identificado." << endl; }
}

void DServer::breakRing()
{
	if (upRingSocket)
		upRingSocket->close();
	if (downRingSocket)
		downRingSocket->close();
	if (downRingConnectedSocket)
		downRingConnectedSocket->close();
}

void DServer::listen()
{
	DMessage* connectionRequest = NULL;
	while(true) {
		bool requestReceived = serverSocket->receive(&connectionRequest);
		if(!requestReceived) continue;
		if(connectionRequest->toString().substr(0,7) == "connect") {
			string clientName = connectionRequest->toString().substr(8, connectionRequest->toString().size());
			mtxClientsListUpdate.lock();
			DClient* client = findClient(clientName);
			mtxClientsListUpdate.unlock();
			if(client == NULL) {
				DClient* newClient = new DClient(clientName);
				if(newClient->bad()) {
					cout << "DServer::listen() - Erro ao criar um novo cliente." << endl;
					serverSocket->reply(new DMessage("connection refused"));
					continue; }
				string clientPath = homeDir + "/dbox-server/" + clientName;
				bool clientPathCreated = (mkdir(clientPath.c_str(), 0777) == 0);
				if(clientPathCreated) {
					mtxClientsListUpdate.lock();
					clients.push_back(newClient);
					mtxClientsListUpdate.unlock();
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
			else if (connectionRequest->toString() == "ping")
				serverSocket->reply(new DMessage("ok")); }
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
			connections.push_back(newConnection);
			bool ackSent = serverSocket->reply(new DMessage("ack port" + to_string(portNumber)));
			if(ackSent) {
				cout << "Conexão aceita. Cliente: " << client->getName() << ". IP: " << inet_ntoa(clientIp) << ". Porta: " << ntohs(clientPort) << "." << endl;
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
	cout << "Envio de arquivo completo. Cliente: " << client->getName() << ". IP: " << inet_ntoa(connection->getDestinationIp()) << ". Arquivo: " << fileName << "." << endl;
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
	ringQueue.push(new DMessage("delete file " + filePath));
	client->getFilesList()->clear();
	client->fillFilesList("/dbox-server/" + client->getName());
	cout << "Arquivo deletado do servidor: " << fileName << ". Cliente: " << client->getName() << "." << endl;
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
	ringQueue.push(new DMessage("receive file " totalPackets + " " filePath));
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
		newFile.write(packet->get(),packet->length());
		ringQueue.push(new DMessage(packet)); }
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
	cout << "Arquivo recebido com sucesso. Cliente: " << client->getName() << ". Arquivo: " << fileName << "." << endl;
	client->updateFilesList(fileName, "/dbox-server/" + client->getName());
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
	client->getConnectionsList()->remove(connection);
	if(client->isConnectedFrom(connection->getDestinationIp())) cout << "Não removido." << endl;
	ports[ntohs(connection->getSocketPort())-SERVER_PORT-1] = AVAILABLE;
	DMessage* closeConfirm = new DMessage("connection closed");
	bool replySent = connection->reply(closeConfirm);
	if(replySent) {
		cout << "Conexão terminada. Cliente: " << client->getName() << ". Dispositivo: " << inet_ntoa(connection->getSenderIp()) << endl;
		connection->closeSocket();
		return true; }
	return false;
}

void DServer::messageProcessing(DClient* client, DSocket* connection)
{
	while(true)
		if (serverStatus == MASTER) {
			DMessage* message = NULL;
			bool messageReceived = connection->receive(&message);
			if(messageReceived) {
				if(message->toString() == "close connection") {
					mtxClientsListUpdate.lock();
					bool connectionClosed = closeConnection(client,connection);
					mtxClientsListUpdate.unlock();
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

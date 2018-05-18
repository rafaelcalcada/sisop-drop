#include "../include/DServer.h"

DServer::DServer()
{
	serverSocket = new DSocket();
	if(serverSocket->isOpen()) {
		bool bindOk = serverSocket->bindSocket("localhost",SERVER_PORT);
		if(bindOk) {
			for(int i = 0; i < MAX_CONNECTIONS; i++) ports[i] = AVAILABLE;
			this->initialize();
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
	const char *aux;
	if((aux = getenv("HOME")) == NULL) aux = getpwuid(getuid())->pw_dir;
	this->homeDir = string(aux);
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
				clients.push_back(newClient); } } }
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

void DServer::listen()
{
	DMessage* connectionRequest = NULL;
	while(true) {
		bool requestReceived = serverSocket->receive(&connectionRequest);
		if(!requestReceived) {
			cout << "DServer::listen() - Erro durante recebimento de pedido de conexão." << endl;
			continue; }
		if(connectionRequest->toString().substr(0,7) == "connect") {
			string clientName = connectionRequest->toString().substr(8, connectionRequest->toString().size());
			DClient* client = findClient(clientName);
			if(client == NULL) {
				DClient* newClient = new DClient(clientName);
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
				else acceptConnection(client, serverSocket->getSenderIp(), serverSocket->getSenderPort()); } } }
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
			bool ackSent = serverSocket->reply(new DMessage("ack " + to_string(portNumber)));
			if(ackSent) {
				cout << "Conexão aceita. Cliente: " << client->getName() << ". IP: " << inet_ntoa(clientIp) << ". Porta: " << clientPort << "." << endl;
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

void DServer::receiveFile(DClient* client, DSocket* connection, DMessage* message)
{
	int fileSize = atoi(message->toString().substr(message->toString().find_last_of(" ")+1).c_str());
	string fileName = message->toString().substr(8, message->toString().find_last_of(" ")-8);
	string filePath = homeDir + "/dbox-server/" + client->getName() + "/" + fileName;
	DMessage* serverReply = new DMessage("receive ack");
	bool serverReplySent = connection->reply(serverReply);
	if(!serverReplySent) {
		cout << "DServer::receiveFile() - Erro ao enviar confirmação para recebimento de arquivo." << endl;
		return; }
	ofstream newFile;
	newFile.open(filePath, ofstream::trunc | ofstream::binary);
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
	cout << "Arquivo recebido com sucesso. Cliente: " << client->getName() << ". Arquivo: " << fileName << "." << endl;
	// TODO: adicionar arquivo na file list do cliente
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
	while(true) {
		DMessage* message = NULL;
		connection->receive(&message);
		if(message->toString() == "close connection") {
			bool connectionClosed = closeConnection(client,connection);
			if(connectionClosed) break;	}             
		if(message->toString().substr(0,7) == "receive") receiveFile(client, connection, message); }		
}

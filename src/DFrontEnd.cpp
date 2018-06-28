#include "../include/DFrontEnd.h"

DFrontEnd::DFrontEnd(string confFile)
{
	currentServer = -1;
	serverReceivedPackets = 0;
	serverReceivedBytes = 0;
	clientReceivedPackets = 0;
	clientReceivedBytes = 0;
	serverCachedMessage = NULL;
	clientCachedMessage = NULL;
	clientPortOffset = 0;

	isWorking = loadConfFile(confFile)
			 && initServerFacedSocket()
			 && initClientFacedSocket();
}

bool DFrontEnd::loadConfFile(string confFile)
{
	cout << "DFrontEnd::loadConfFile() - Lendo endereços de \"" << confFile << "\"." << endl;
	ifstream infile;
	try {
		infile.open(confFile); }
	catch (const exception &e) {
		cout << "DFrontEnd::loadConfFile() - Exception. Lançado " << e.what() << " ao abrir o arquivo de configuração para leitura." << endl;
		return false; }

	string confLine;
	int numServers = 0;
	string ip;
	int port;
	while (!infile.eof()) {
		infile >> confLine;
		if (confLine != "") {
			int colon = confLine.find(':');
			try {
				ip = confLine.substr(0, colon);
				port = stoi(confLine.substr(colon+1)); }
			catch (const exception &e) {
				cout << "DFrontEnd::loadConfFile() - Exception. Lançado " << e.what() << " processando endereços de servidor." << endl;
				return false; }
			if (!validateAddress(ip, port))
				cout << "DFrontEnd::loadConfFile() - Erro. \"" << confLine << "\" não é um endereço ip:porta válido." << endl;
			pair<string, int> newServer = make_pair(ip, port);
			servers.push_back(newServer);

			numServers++;
			cout << "DFrontEnd::loadConfFile() - Servidor #" << numServers << "  " << ip << ":" << port << endl; } }
	if (numServers == 0) {
		cout << "DFrontEnd::loadConfFile() - Erro. Nenhum servidor adicionado." << endl; 
		return false; }
	else if (numServers == 1)
		cout << "DFrontEnd::loadConfFile() - Aviso. Apenas um servidor encontrado. Recuperação em caso de falhas não será possível." << endl;

	return true;
}

bool DFrontEnd::validateAddress(string ip, int port)
{
	if ((port < 0) || (port > 65535))
		return false;
	else {
		struct sockaddr_in sa;
		return (inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1);
	}
	return true;
}

bool DFrontEnd::initServerFacedSocket()
{
	serverFacedSocket = new DSocket();
	if(serverFacedSocket->isOpen()) {
		bool bindOk = serverFacedSocket->bindSocket("localhost", FRONT_END_SERVER_FACED_PORT);
		if(bindOk)
			return true;
		else
			cout << "DFrontEnd::initClientFacedSocket() - Erro. Chamada de serverSocket->bind() retornou código de erro (" << errno << ")." << endl; }
	else
		cout << "DFrontEnd::initServerFacedSocket() - Erro. Chamada de serverSocket->isOpen() retornou código de erro." << endl;
	return false;
}

bool DFrontEnd::initClientFacedSocket()
{
	clientFacedSocket = new DSocket();
	if(clientFacedSocket->isOpen()) {
		bool bindOk = clientFacedSocket->bindSocket("localhost", FRONT_END_CLIENT_FACED_PORT);
		if(!bindOk) {
			cout << "DFrontEnd::initClientFacedSocket() - Erro. Chamada de serverSocket->bind() retornou código de erro (" << errno << ")." << endl;
			clientFacedSocket->closeSocket();
			return false; }
		return true; }
	else
		cout << "DFrontEnd::initClientFacedSocket() - Erro. Chamada de serverSocket->isOpen() retornou código de erro." << endl;
	return false;
}

void DFrontEnd::printStatus()
{
	if (currentServer >= 0) {
		cout << "Servidor atual: #" << (currentServer + 1) << " - "
			 << get<POS_IP>(servers[currentServer])  << ":"
			 << get<POS_PORT>(servers[currentServer])  << endl; }
	else
		cout << "Sem conexão com o servidor" << endl;
	cout << "Recebido do servidor: " << serverReceivedPackets << " pacotes, " << serverReceivedBytes << " bytes" << endl
		 << "Recebido do cliente: " << clientReceivedPackets << " pacotes, " << clientReceivedBytes << " bytes" << endl;
}

void DFrontEnd::mainLoop()
{
	while (true) {
		while (!receiveFromClient())
			;
		if (currentServer == -1)
			connectServer();
		while (!sendReceiveFromServer()) {
			cout << "DFrontEnd::mainLoop() - Erro. Envio de mensagem para o servidor falhou. Iniciando reconexão." << endl;
			if (clientPortOffset > 0)
				serverFacedSocket->setDestination(get<POS_IP>servers[currentServer], get<POS_PORT>servers[currentServer]);
			while (!connectServer()) {
				cout << "DFrontEnd::mainLoop() - Erro. chamada de connectServer encontrou erro." << endl
					 << "Pressione qualquer tecla para tentar a reconexão." << endl;
				getchar(); }
				if (clientPortOffset > 0)
					serverFacedSocket->setDestination(get<POS_IP>servers[currentServer], (get<POS_PORT>servers[currentServer] + clientPortOffset)); }
		sendToClient(); } }

bool DFrontEnd::connectServer()
{
	cout << "DFrontEnd::connectServer() - Iniciando busca por servidor." << endl;
	DMessage* ping = new DMessage("ping");
	DMessage* reply = NULL;
	for (int i = 0; i < servers.size(); i++) {
		cout << "DFrontEnd::connectServer() - Conectando ao servidor #" << (i + 1) << " - "
			 << get<POS_IP>(servers[i])  << ":"
			 << get<POS_PORT>(servers[i])  << endl;
		serverFacedSocket->setDestination(get<POS_IP>servers[i], get<POS_PORT>(servers[i]));
		for (int j = 0; j < MAX_RETRIES; j++) {
			cout << ".";
			serverFacedSocket->sendMessage(ping);
			if (serverFacedSocket->receiveMessage(&reply) && (reply->toString() =="OK" )) {
				cout << " OK" << endl;
				saveReceivedServerMessage(reply);
				return true; } }
		cout << " sem resposta do servidor." << endl; }
	cout << "DFrontEnd::connectServer() - Erro. Nenhum servidor respondeu à tentativa de conexão." << endl;
	return false;
}

bool DFrontEnd::sendReceiveFromServer()
{
	for (int i = 0; i < MAX_RETRIES; i++) {
		if (serverFacedSocket->sendMessage(clientCachedMessage)) {
			DMessage* message = NULL;
			if (serverFacedSocket->receiveMessage(&message)) {
				if (message->toString().substr(0,8) == "ack port") {
					int newServerPort = atoi(serverReply->toString().substr(4).c_str());
					clientPortOffset = newServerPort - get<POS_PORT>servers[currentServer];
					serverFacedSocket->setDestination(get<POS_IP>servers[currentServer], newServerPort);
					delete message;
					message = new DMessage("ack port" + to_string(FRONT_END_CLIENT_FACED_PORT)); }
				else if (message->toString() == "connection closed")
					clientPortOffset = 0;
					serverFacedSocket->setDestination(get<POS_IP>servers[currentServer], get<POS_PORT>servers[currentServer]);
				saveReceivedServerMessage(message);
				return true; }
			else {
				cout << "DFrontEnd::sendReceiveFromServer() - Erro. Recebimento de mensagem retornou erro (" << errno << ")." << endl;
				return false; } }
		else
			cout << "DFrontEnd::sendReceiveFromServer() - Erro. Envio de mensagem retornou erro (" << errno << ")." << endl; }
	cout << "DFrontEnd::sendReceiveFromServer() - Erro. Limite de tentativas excedido." << endl;
	return false;
}

bool DFrontEnd::receiveFromClient()
{
	DMessage* message = NULL;
	bool retry = true;
	while (!clientFacedSocket->receiveMessage(&message)) {
		if ((errno != EAGAIN) || (errno != EWOULDBLOCK)) {
			cout << "DFrontEnd::receiveFromClient() - Erro. Recebimento de mensagem retornou erro (" << errno << ")." << endl;
			if (message)
				delete message;
			return false;
		}
	}
	saveReceivedClientMessage(message);
	delete message;
	return true;
}

void DFrontEnd::sendToClient()
{
	if (!clientFacedSocket->sendMessage(serverCachedMessage))
		cout << "DFrontEnd::sendToClient() - Erro. Envio de mensagem retornou erro (" << errno << ")." << endl;
}

void DFrontEnd::saveReceivedServerMessage(DMessage* message)
{
	serverReceivedPackets++;
	serverReceivedBytes += message->length();
	if (serverCachedMessage)
		delete serverCachedMessage;
	serverCachedMessage = new DMessage(message->get());
}

void DFrontEnd::saveReceivedClientMessage(DMessage* message)
{
	clientReceivedPackets++;
	clientReceivedBytes += message->length();
	if (clientCachedMessage)
		delete clientCachedMessage;
	clientCachedMessage = new DMessage(message->get());
}

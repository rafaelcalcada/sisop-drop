#include "../include/DClient.h"

DClient::DClient(string clientName)
{
	const char *aux;
	if((aux = getenv("HOME")) == NULL) aux = getpwuid(getuid())->pw_dir;
	this->homeDir = string(aux);
	this->clientName = clientName;
	clientSocket = new DSocket();
	bool socketOpen = clientSocket->isOpen();
	if(socketOpen) isWorking = true;
	else isWorking = false;
}

bool DClient::createSyncDir()
{
	string clientPath = homeDir + "/sync_dir_" + clientName;
	struct stat st;
	if(stat(clientPath.c_str(), &st) == -1) {
		bool clientDirCreated = (mkdir(clientPath.c_str(), 0777) == 0);
		if(!clientDirCreated) return false;
		else return true; }
	else return true;
}

void DClient::help()
{
	cout << endl << "\e[1mCOMANDOS\e[0m" << endl << endl;
	cout << "\e[1mhelp\e[0m\nComando que ajuda com todos os comandos" << endl << endl;
	cout << "\e[1mdownload\e[0m\nFaz o download de um arquivo do servidor DBox.\nSintaxe: download [pathtofile]" << endl << endl;
	cout << "\e[1mupload\e[0m\nFaz o upload de um arquivo para o servidor DBox.\nSintaxe: upload [pathtofile]"<< endl << endl;
	cout << "\e[1mquit\n\e[0mEncerra a conexão com o servidor e fecha o DBox." << endl << endl << "> ";
}

bool DClient::isConnectedFrom(struct in_addr ipAddress)
{
	list<DSocket*>::iterator it;
	for(it = connections.begin(); it != connections.end(); ++it) {
		DSocket* conn = *(it);
		if(conn->getDestinationIp().s_addr == ipAddress.s_addr) return true; }
	return false;
}

bool DClient::connect(const char* serverAddress, int serverPort)
{
	bool destinationSetted = clientSocket->setDestination(serverAddress,serverPort);
	if(!destinationSetted) {
		cout << "DClient::connect() - Erro ao configurar endereço de destino do socket do cliente." << endl;
		return false; }
	DMessage* connectionRequest = new DMessage("connect " + clientName);
	bool requestSent = clientSocket->send(connectionRequest);
	if(!requestSent) {
		cout << "DClient::connect() - Erro ao enviar pedido de conexão." << endl;
		return false; }
	DMessage* serverReply = NULL;
	bool replyReceived = clientSocket->receive(&serverReply);
	if(!replyReceived) {
		cout << "DClient::connect() - Erro ao receber resposta do servidor." << endl;
		return false; }
	if(serverReply->toString() == "connection refused") {
		cout << "Conexão recusada pelo servidor." << endl;
		return false; }
	if(serverReply->toString().substr(0,3) == "ack") {
		int newServerPort = atoi(serverReply->toString().substr(4).c_str());
		bool serverPortChanged = clientSocket->setDestination(serverAddress,newServerPort);
		if(serverPortChanged) {
			cout << "Conexão aceita pelo servidor na porta " << newServerPort << "." << endl;
			return true; } }
	return false;
}

bool DClient::sendFile(string filePath)
{
	ifstream file;
	file.open(filePath, ifstream::in | ifstream::binary);
	if(!file.is_open()) {
		cout << "Erro: arquivo não encontrado." << endl;
		return false; }
	file.seekg(0, file.end); 
	int fileSize = file.tellg();
	file.seekg(0, file.beg);
	DMessage* sendRequest = new DMessage("receive " + filePath.substr(filePath.find_last_of("/")+1) + " " + to_string(fileSize));
	bool requestSent = clientSocket->send(sendRequest);
	if(!requestSent) {
		cout << "DClient::sendFile() - Erro. Requisição de envio não enviada." << endl;
		return false; }
	DMessage* serverReply = NULL;
	bool replyReceived = clientSocket->receive(&serverReply);
	if(!replyReceived) {
		cout << "DClient::sendFile() - Erro ao receber resposta da requisição." << endl;
		return false; }
	if(serverReply->toString() != "receive ack") {
		cout << "Erro. Servidor recusou recebimento do arquivo." << endl;
		return false; }
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
		bool packetSent = clientSocket->send(packet);
		if(!packetSent) {
			cout << "DClient::sendFile() - Erro ao enviar pacote para o servidor. Envio interrompido." << endl;
			return false; }
		DMessage* packetDeliveryStatus = NULL;
		bool packetDeliveryResponseReceived = clientSocket->receive(&packetDeliveryStatus);
		if(!packetDeliveryResponseReceived) {
			cout << "DClient::sendFile() - Erro. Confirmação de entrega de pacote não recebida. Envio interrompido." << endl;
			return false; }
		if(packetDeliveryStatus->toString() == "packet received") continue;
		else {
			cout << "DClient::sendFile() - Erro. Status de entrega de pacote desconhecido." << endl;
			return false; } }
	file.close();
	return true;
}

bool DClient::receiveFile(string fileName)
{
	DMessage* receiveRequest = new DMessage("send " + fileName);
	bool requestSent = clientSocket->send(receiveRequest);
	if(!requestSent) {
		cout << "DClient::receiveFile() - Erro. Requisição de recebimento não enviada." << endl;
		return false; }
	DMessage* serverReply = NULL;
	bool replyReceived = clientSocket->receive(&serverReply);
	if(!replyReceived) {
		cout << "DClient::receiveFile() - Erro. Resposta do servidor não recebida." << endl;
		return false; }
	if(replyReceived && serverReply->toString() == "file not found") {
		cout << "Resposta do servidor: Arquivo não encontrado." << endl;
		return false; }
	if(replyReceived && serverReply->toString().substr(0,8) != "send ack") {
		cout << "DClient::receiveFile() - Erro. Resposta desconhecida." << endl;
		return false; }
	int fileSize = atoi(serverReply->toString().substr(9).c_str());
	string filePath = homeDir + "/sync_dir_" + clientName + "/" + fileName;
	ofstream newFile;
	newFile.open(filePath, ofstream::trunc | ofstream::binary);
	if(!newFile.is_open()) {
		cout << "DClient::receiveFile() - Erro ao receber arquivo. Não foi possível criar cópia local." << endl;
		return false; }
	DMessage* confirm = new DMessage("send ack ack");
	bool confirmationSent = clientSocket->send(confirm);
	if(!confirmationSent) {
		cout << "DClient::receiveFile() - Erro. Confirmação para recebimento não enviada." << endl;
		return false; }
	int totalPackets = (int)((fileSize/BUFFER_SIZE)+1);
	int packetsReceived = 0;
	int packetSize = 0;
	DMessage* packet = NULL;
	while(packetsReceived < totalPackets) {
		packetsReceived++;				
		bool packetReceived = clientSocket->receive(&packet);
		if(!packetReceived) {
			cout << "DClient::receiveFile() - Erro ao receber pacote do arquivo. Recebimento interrompido." << endl;
			newFile.close();
			return false; }
		DMessage* confirmation = new DMessage("packet received");
		bool confirmationSent = clientSocket->reply(confirmation);
		if(!confirmationSent) {
			cout << "DClient::receiveFile() - Erro ao enviar confirmação de recebimento de pacote de dados. Recebimento interrompido." << endl;
			newFile.close();
			return false; }
		newFile.write(packet->get(),packet->length()); }
	newFile.close();
	// TODO: adicionar arquivo na file list do cliente
	return true;
}

bool DClient::closeConnection()
{
	bool closeRequestSent = clientSocket->send(new DMessage("close connection"));
	if(closeRequestSent) {		
		DMessage* serverReply = NULL;
		bool serverReplyReceived = clientSocket->receive(&serverReply);
		if(serverReplyReceived && serverReply->toString() == "connection closed") {
			bool clientSocketClosed = clientSocket->closeSocket();
			return clientSocketClosed; }
		else return false; }
	else return false;
}

#include "../include/DSocket.h"

DSocket::DSocket(DSocketType type)
{
	socketType = type;
	if (type == UDP)
		sock = socket(AF_INET, SOCK_DGRAM, 0);
	else if (type == TCP)
		sock = socket(AF_INET, SOCK_STREAM, 0);
	else
		_isOpen = false;

	if (sock == -1)
		_isOpen = false;
	else if (_isOpen) {
		_isOpen = true;
		struct timeval timeout;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) cout << "Erro ao estabelecer timeout para receive." << endl;
		if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) cout << "Erro ao estabelecer timeout para send." << endl; }
}

DSocket::DSocket(int newSocket, sockaddr_in newAddress, sockaddr_in newDestAddress) {
	address = newAddress;
	destAddress = newDestAddress;
	sock = newSocket;
	socketType = TCP;
	_isOpen = true;
}

bool DSocket::bindSocket(const char* ipAddress, int portNumber)
{
	if(string(ipAddress) == "localhost") address.sin_addr.s_addr = INADDR_ANY;
	else {
		if(inet_aton(ipAddress, &(address.sin_addr)) == 0) {
		cout << "DSocket::bind() -  Erro ao converter string para IP." << endl;
		this->closeSocket();
		return false; } }
	address.sin_family = AF_INET;
	address.sin_port = htons(portNumber);
	if(bind(sock, (struct sockaddr *) &address, sizeof(struct sockaddr)) == 0) return true;
	else {
		cout << "DSocket::bind() - Chamada bind() retornou código de erro." << endl;
		this->closeSocket();
		return false; }
}

bool DSocket::listenSocket() {
	return listen(sock, BACKLOG_SIZE);
}

bool DSocket::connectSocket() {
    int addrSize;
	return (connect(sock, (struct sockaddr *)&destAddress, (socklen_t)addrSize) != -1);
}

DSocket* DSocket::acceptConnection() {
	int serverAddrSize;
	struct sockaddr_in serverAddress;
	int s = accept(sock, (struct sockaddr *)&serverAddress, (socklen_t*)&serverAddrSize);
	if (s == -1)
		return NULL;
	else
		return new DSocket(s, address, serverAddress);
}

bool DSocket::closeSocket()
{
	int flag = close(sock);
	if(flag == 0 || flag == EBADF) return true;
	else {
		cout << "DSocket::closeSocket() - Erro ao fechar socket." << endl;
		return false; }
}

bool DSocket::setDestination(const char* ipAddress, int portNumber)
{
	string ip(ipAddress);
	if(ip == "localhost") ip = "127.0.0.1";
	if(inet_aton(ip.c_str(), &(destAddress.sin_addr)) == 0)	{
		cout << "DSocket::setDestination() - Erro ao converter string para IP." << endl;
		return false; }
	else {
		destAddress.sin_family = AF_INET;
		destAddress.sin_port = htons(portNumber); 
		return true; }
}

bool DSocket::sendMessage(DMessage* message)
{
    int flag;
	if (socketType == UDP)
		flag = sendto(sock, (const void*) message->get(), (size_t) message->length(), 0, (struct sockaddr *) &destAddress, sizeof(struct sockaddr));
	else if (socketType == TCP)
		flag = send(sock, (const void*) message->get(), (size_t) message->length(), 0);
	else
		return false;
	if (flag < 0) return false;
	else return true;
}

bool DSocket::replyMessage(DMessage* message)
{
    int flag;
	if (socketType == UDP)
		flag = sendto(sock, (const void*) message->get(), (size_t) message->length(), 0, (struct sockaddr *) &senderAddress, sizeof(struct sockaddr));
	else if (socketType == TCP)
		flag = send(sock, (const void*) message->get(), (size_t) message->length(), 0);
	else
		return false;
	if(flag < 0) return false;
	else return true;
}

bool DSocket::receiveMessage(DMessage** message) {
	char* msgBuffer = new char[BUFFER_SIZE];
	socklen_t sockAddressSize = sizeof(struct sockaddr);
    int msgSize;
	if (socketType == UDP)
		msgSize = recvfrom(sock, (void*) msgBuffer, BUFFER_SIZE, 0, (struct sockaddr *) &senderAddress, &sockAddressSize);
	else if (socketType == TCP)
		msgSize = recv(sock, (void*) msgBuffer, BUFFER_SIZE, 0);
	else
		return false;
	if(msgSize < 0) return false;
	else {
		*message = new DMessage(msgBuffer, msgSize);
		return true; }
}

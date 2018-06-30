#include "../include/DSocket.h"

DSocket::DSocket()
{
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) cout << "Erro ao estabelecer timeout para receive." << endl;
    if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) cout << "Erro ao estabelecer timeout para send." << endl;
	if(sock == -1) _isOpen = false;
	else _isOpen = true;
}

void DSocket::setTimeOut(int secs)
{
	struct timeval timeout;      
    timeout.tv_sec = secs;
    timeout.tv_usec = 0;
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) cout << "Erro ao estabelecer timeout para receive." << endl;
	if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) cout << "Erro ao estabelecer timeout para send." << endl;
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
		cout << "DSocket::bind() - Chamada bind() retornou código de erro. IP " << ipAddress << ":" << portNumber << endl;
		if(errno == EADDRINUSE) cout << "Endereço em uso." << endl;
		this->closeSocket();
		return false; }
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

bool DSocket::setSender(const char* ipAddress, int portNumber)
{
	string ip(ipAddress);
	if(ip == "localhost") ip = "127.0.0.1";
	if(inet_aton(ip.c_str(), &(senderAddress.sin_addr)) == 0)	{
		cout << "DSocket::setSender() - Erro ao converter string para IP." << endl;
		return false; }
	else {
		senderAddress.sin_family = AF_INET;
		senderAddress.sin_port = htons(portNumber); 
		return true; }
}

bool DSocket::send(DMessage* message)
{
	//cout << "send " << inet_ntoa(this->getDestinationIp()) << ":" << ntohs(this->getDestinationPort()) << " " << message->get() << endl;
	//cout.flush();
	int flag = sendto(sock, (const void*) message->get(), (size_t) message->length(), 0, (struct sockaddr *) &destAddress, sizeof(struct sockaddr));
	if(flag < 0) return false;
	else return true;
}

bool DSocket::reply(DMessage* message)
{
	//cout << "reply " << inet_ntoa(this->getSenderIp()) << ":" << ntohs(this->getSenderPort()) << " " << message->get() << endl;
	//cout.flush();
	int flag = sendto(sock, (const void*) message->get(), (size_t) message->length(), 0, (struct sockaddr *) &senderAddress, sizeof(struct sockaddr));
	if(flag < 0) return false;
	else return true;
}

bool DSocket::receive(DMessage** message) {
	char* msgBuffer = new char[BUFFER_SIZE];
	socklen_t sockAddressSize = sizeof(struct sockaddr);
	int msgSize = recvfrom(sock, (void*) msgBuffer, BUFFER_SIZE, 0, (struct sockaddr *) &senderAddress, &sockAddressSize);
	if(msgSize < 0) return false;
	else {
		*message = new DMessage(msgBuffer, msgSize);
		//cout << "receive " << inet_ntoa(this->getSenderIp()) << ":" << ntohs(this->getSenderPort()) << " " << (*message)->get() << endl;
		//cout.flush();
		return true;
	}
}

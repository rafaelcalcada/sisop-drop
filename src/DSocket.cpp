#include "../include/DSocket.h"

DSocket::DSocket()
{
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1) _isOpen = false;
	else _isOpen = true;
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

bool DSocket::send(DMessage* message)
{
	int flag = sendto(sock, (const void*) message->get(), (size_t) message->length(), 0, (struct sockaddr *) &destAddress, sizeof(struct sockaddr));
	if(flag < 0) { // significa que um erro ocorreu
		cout << "DSocket::send() - Erro. Não foi possível enviar a mensagem." << endl;
		return false; }
	else return true;
}

bool DSocket::reply(DMessage* message)
{
	int flag = sendto(sock, (const void*) message->get(), (size_t) message->length(), 0, (struct sockaddr *) &senderAddress, sizeof(struct sockaddr));
	if(flag < 0) { // significa que um erro ocorreu
		cout << "DSocket::reply() - Erro. Não foi possível responder a mensagem." << endl;
		return false; }
	else return true;
}

bool DSocket::receive(DMessage** message) {
	char* msgBuffer = new char[BUFFER_SIZE];
	socklen_t sockAddressSize = sizeof(struct sockaddr);
	int msgSize = recvfrom(sock, (void*) msgBuffer, BUFFER_SIZE, 0, (struct sockaddr *) &senderAddress, &sockAddressSize);
	if(msgSize < 0) {
		cout << "DSocket::receive() - Erro. Não foi possível receber corretamente a mensagem." << endl;
		return false; }
	else {
		*message = new DMessage(msgBuffer, msgSize);
		return true;
	}
}

#ifndef DSOCKET_H
#define DSOCKET_H

#include <iostream>
#include <string>
#include "DMessage.h"

// cabeçalhos para trabalhar com sockets
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

const int BUFFER_SIZE = 500;

class DSocket {
	
private:
	int sock;
	struct sockaddr_in address;
	struct sockaddr_in senderAddress;	
	struct sockaddr_in destAddress;
	bool _isOpen;
	
public:
	DSocket(); // OK
	bool bindSocket(const char* ipAddress, int portNumber); // OK
	bool isOpen() { return _isOpen; } // OK
	bool setDestination(const char* ipAddress, int portNumber); // OK
	bool send(DMessage* message); // OK
	bool receive(DMessage** message); // OK
	bool reply(DMessage* message); // OK
	bool closeSocket(); // OK
	struct in_addr getSocketIp() { return address.sin_addr; } // OK
	unsigned short getSocketPort() { return address.sin_port; } // OK	
	struct in_addr getDestinationIp() { return destAddress.sin_addr; } // OK
	unsigned short getDestinationPort() { return destAddress.sin_port; } // OK
	struct in_addr getSenderIp() { return senderAddress.sin_addr; } // OK
	unsigned short getSenderPort() { return senderAddress.sin_port; } // OK
	
};

#endif

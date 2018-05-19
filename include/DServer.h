#ifndef DSERVER_H
#define DSERVER_H

#include <iostream>
#include <fstream>
#include <list>
#include <thread>
#include <string>
#include <cstring>

#include "DClient.h"
#include "DSocket.h"

#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>

using namespace std;

const int SERVER_PORT = 50000;
const int MAX_CONNECTIONS = 1000;

enum DPortStatus { AVAILABLE, UNAVAILABLE, OCCUPIED };

class DServer {

private:
	list<DClient*> clients;
	list<thread*> connections;
	DPortStatus ports[MAX_CONNECTIONS];
	DSocket* serverSocket;
	bool isWorking;
	string homeDir;
	
public:
	DServer(); // OK
	DClient* findClient(string clientName);
	bool bad() { return !isWorking; } // OK
	void listen();
	void acceptConnection(DClient* client, struct in_addr clientIp, unsigned short clientPort);
	bool closeConnection(DClient* client, DSocket* connection);
	void messageProcessing(DClient* client, DSocket* connection);
	void initialize(); // OK
	bool sendFile(DClient* client, DSocket* connection, DMessage* message);
	void receiveFile(DClient* client, DSocket* connection, DMessage* message);
	int getAvailablePort();

};

#endif


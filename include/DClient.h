#ifndef DCLIENT_H
#define DCLIENT_H

#include <iostream>
#include <fstream>
#include <list>
#include <string>
using namespace std;

#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>

#include "DSocket.h"

class DClient {
	
private:
	list<DSocket*> connections;
	//list<DFile*> files;
	DSocket* clientSocket;
	string clientName;
	string homeDir;
	bool isWorking;
	
public:
	DClient() { isWorking = false; } // OK
	DClient(string clientName); // OK
	string getName() { return clientName; } // OK
	bool bad() { return !isWorking; } // OK
	void help(); // OK
	bool createSyncDir(); // OK
	bool connect(const char* serverAddress, int serverPort); // OK
	bool isConnected() { return !connections.empty(); } // OK
	bool isConnectedFrom(struct in_addr ipAddress); // OK
	bool sendFile(string filePath); // OK
	bool receiveFile(string fileName);
	bool closeConnection(); // OK
	list<DSocket*>* getConnectionsList() { return &connections; } // OK
	//list<DFile*>* getFilesList() { return &files; }

};

#endif

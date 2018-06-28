#ifndef DSERVER_H
#define DSERVER_H

#include <iostream>
#include <fstream>
#include <list>
#include <thread>
#include <mutex>
#include <string>
#include <cstring>

#include "DClient.h"
#include "DSocket.h"

#include <sys/stat.h>
#include <utime.h>
#include <dirent.h>
#include <pwd.h>

using namespace std;

const int SERVER_PORT = 50000;
const int MAX_CONNECTIONS = 1000;
const int MAX_RETRIES = 5;
const int POS_IP = 0;
const int POS_PORT = 1;

enum DPortStatus { AVAILABLE, UNAVAILABLE, OCCUPIED };
enum DServerStatus { MASTER, SLAVE, OFFLINE };

class DServer {

private:
	list<DClient*> clients;
	list<thread*> connections;
	DPortStatus ports[MAX_CONNECTIONS];
	DSocket* serverSocket;
	bool isWorking;
	string homeDir;
	mutex mtxClientsListUpdate;
	vector<pair<string, int> > servers;
	int thisServerID;
	string thisServerAddresses;
	DServerStatus serverStatus;
	DSocket* upRingSocket;
	DSocket* upRingConnectedSocket;
	DSocket* downRingSocket;
	bool isUpRingConnected;
	bool isDownRingConnected;
	DMessage* ringMessage;
	queue<DMessage*> ringQueue;
	string receivingFilePath;
	int receivingFilePartsRemaining;

	bool loadConfFile(string confFile); // OK
    bool validateAddress(string ip, int port); // OK
	bool setUpRing(); // OK
	string getIpAddressList(); // OK
    bool initializeRing(); // OK
	bool connectRing(); // OK
    int getSocketServerID(DSocket* socket); // OK
	bool processUpdate(); // OK
	void breakRing(); // OK

public:
	DServer(string confFile); // OK
	DClient* findClient(string clientName); // OK
	bool bad() { return !isWorking; } // OK
	void coordinate(); // OK
	void listen(); // OK
	void acceptConnection(DClient* client, struct in_addr clientIp, unsigned short clientPort); // OK
	bool closeConnection(DClient* client, DSocket* connection); // OK
	void messageProcessing(DClient* client, DSocket* connection); // OK
	bool initialize(); // OK
	void sendFile(DClient* client, DSocket* connection, DMessage* message); // OK
	void receiveFile(DClient* client, DSocket* connection, DMessage* message); // OK
	void deleteFile(DClient* client, DSocket* connection, DMessage* message); // OK
	void listFiles(DClient* client, DSocket* connection, DMessage* message); // OK
	void listClientFiles(string clientName); // OK
	void clientsFileListUpdaterDaemon(); // OK
	int getAvailablePort(); // OK

};

#endif


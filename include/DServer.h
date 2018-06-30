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

enum DPortStatus { AVAILABLE, UNAVAILABLE, OCCUPIED };

struct ReplicaManager {
	struct in_addr address;
	unsigned int id;
	bool primary;
};

class DServer {

private:
	list<ReplicaManager*> replicaManagers;
	list<DClient*> clients;
	list<thread*> connections;
	list<DSocket*> tempsocks;
	list<DSocket*> clisocks;
	DPortStatus ports[MAX_CONNECTIONS];
	DSocket* serverSocket;
	string homeDir;
	string primaryIp;
	bool electionRunning;
	bool isPrimary;
	bool isWorking;
	unsigned int serverId;
	
public:
	DServer(); // OK
	DClient* findClient(string clientName); // OK
	bool bad() { return !isWorking; } // OK
	void listen(); // OK
	void acceptConnection(DClient* client, struct in_addr clientIp, unsigned short clientPort); // OK
	bool closeConnection(DClient* client, DSocket* connection); // OK
	bool notifyPrimary();
	bool updatePrimary();
	void setPrimary(string _primaryIp);
	void startBackup(DSocket* connSocket);
	void updateBackup(DSocket* connSocket);
	void closeSocketAndFreePort(DSocket* connSocket);
	void startElection();
	void auditElection(DSocket* socket, int* voters);
	void messageProcessing(DClient* client, DSocket* connection); // OK
	void initialize(); // OK
	void sendFile(DClient* client, DSocket* connection, DMessage* message); // OK
	void receiveFile(DClient* client, DSocket* connection, DMessage* message); // OK
	void deleteFile(DClient* client, DSocket* connection, DMessage* message); // OK
	void listFiles(DClient* client, DSocket* connection, DMessage* message); // OK
	void listClientFiles(string clientName); // OK
	void clientsFileListUpdaterDaemon(); // OK
	void syncDaemon(); // OK
	void sendRepWarning(DSocket* socket); // OK
	void syncWithPrimary(PrintOption option); // OK
	void notifyReplicas(); // OK
	int getAvailablePort(); // OK
	unsigned int getId() { return serverId; } // OK

};

#endif


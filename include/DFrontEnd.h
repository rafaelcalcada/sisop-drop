#ifndef DFRONTEND_H
#define DFRONTEND_H

#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <utility>

#include "DSocket.h"

using namespace std;

const int FRONT_END_SERVER_FACED_PORT = 51000;
const int FRONT_END_CLIENT_FACED_PORT = 52000;
const int POS_IP = 0;
const int POS_PORT = 1;
const int MAX_RETRIES = 5;

enum DPortStatus { AVAILABLE, UNAVAILABLE, OCCUPIED };

class DFrontEnd {

private:
	DSocket* serverFacedSocket;
	DSocket* clientFacedSocket;
	bool isWorking;
	vector<pair<string, int> > servers;
	int currentServer;
	long serverReceivedPackets;
	long serverReceivedBytes;
	long clientReceivedPackets;
	long clientReceivedBytes;
	DMessage* serverCachedMessage;
	DMessage* clientCachedMessage;
	int clientPortOffset;
	
	bool loadConfFile(string confFile);
	bool validateAddress(string ip, int port);
	bool initClientFacedSocket();
	bool initServerFacedSocket();
	
public:
	DFrontEnd(string confFile);
	bool bad() { return !isWorking; }
	void printStatus();
	void mainLoop();
	bool connectServer();
	bool sendReceiveFromServer();
	bool receiveFromClient();
	void sendToClient();
	void saveReceivedServerMessage(DMessage* message);
	void saveReceivedClientMessage(DMessage* message);
};

#endif


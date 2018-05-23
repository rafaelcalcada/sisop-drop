#ifndef DCLIENT_H
#define DCLIENT_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <list>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
using namespace std;

#include <sys/stat.h>
#include <utime.h>
#include <dirent.h>
#include <pwd.h>

#include "DSocket.h"
#include "DFile.h"

enum PrintOption { PRINT, DONT_PRINT };
enum DeleteLocalFileOption { DELETE, DONT_DELETE };
enum FileCopyOption { COPY, DONT_COPY };

class DClient {
	
private:
	list<DSocket*> connections;
	list<DFile*> files;
	list<DFile*> serverFiles;
	DSocket* clientSocket;
	string clientName;
	string homeDir;
	bool isWorking;
	// mutex para controle de concorrência no servidor e controle de acesso à conexão no cliente
	mutex mtxClient;
	
public:
	DClient() { isWorking = false; } // OK
	DClient(string clientName); // OK
	string getName() { return clientName; } // OK
	DFile* findFile(list<DFile*>* fileList, string fileName); // OK
	void help(); // OK
	void listFiles(); // OK
	void autoUpload(); // OK
	void synchronize(); // OK
	void fileUpdaterDaemon(); // OK
	bool bad() { return !isWorking; } // OK
	bool createSyncDir(); // OK
	bool connect(const char* serverAddress, int serverPort); // OK
	bool isConnected() { return !connections.empty(); } // OK
	bool isConnectedFrom(struct in_addr ipAddress); // OK
	bool sendFile(string filePath, FileCopyOption option); // OK
	bool receiveFile(string fileName); // OK
	bool deleteFile(string fileName, DeleteLocalFileOption option); // OK
	bool deleteLocalFile(string fileName); // OK
	bool closeConnection(); // OK
	bool fillFilesList(string basePath); // OK
	bool updateFilesList(string newFileName, string basePath); // OK
	bool listServerFiles(PrintOption printOnScreen); // OK
	mutex* getMutex() { return &mtxClient; } // OK
	list<DSocket*>* getConnectionsList() { return &connections; } // OK
	list<DFile*>* getFilesList() { return &files; } // OK

};

#endif

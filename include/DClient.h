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
	DSocket* updateSocket;
	string clientName;
	string homeDir;
	bool isWorking;
	// mutex para controle de concorrência no servidor e controle de acesso à conexão no cliente
	mutex mtxClient;
	
public:
	DClient() { isWorking = false; } // OK
	DClient(string clientName); // OK
	DClient(string clientName, bool bind); // OK
	string getName() { return clientName; } // OK
	DFile* findFile(list<DFile*>* fileList, string fileName); // OK
	void listen(); // OK
	void help(); // OK
	void listFiles(); // OK
	void autoUpload(string basePath); // OK
	void synchronize(string basePath, PrintOption option); // OK
	void fileUpdaterDaemon(string basePath); // OK
	void updateReverse(string basePath); // OK
	bool bad() { return !isWorking; } // OK
	bool createSyncDir(string basePath); // OK
	bool connect(const char* serverAddress, int serverPort); // OK
	bool isConnected() { return !connections.empty(); } // OK
	bool isConnectedFrom(struct in_addr ipAddress); // OK
	bool sendFile(string basePath, string filePath, FileCopyOption option); // OK
	bool receiveFile(string basePath, string fileName); // OK
	bool deleteFile(string basePath, string fileName, DeleteLocalFileOption option); // OK
	bool deleteLocalFile(string basePath, string fileName); // OK
	bool closeConnection(); // OK
	bool fillFilesList(string basePath); // OK
	bool syncList(string basePath); // OK
	bool updateFilesList(string newFileName, string basePath); // OK
	bool listServerFiles(PrintOption printOnScreen); // OK
	mutex* getMutex() { return &mtxClient; } // OK
	list<DSocket*>* getConnectionsList() { return &connections; } // OK
	list<DFile*>* getFilesList() { return &files; } // OK

};

#endif


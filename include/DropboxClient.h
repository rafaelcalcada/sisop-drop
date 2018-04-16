#include <iostream>
#include <string>
#include <string.h>

using namespace std;

const int MAX_NUM_FILES = 1024;
const int MAX_DEVICES = 2;

class FileInfo {

	private:
		string fileName;
		string extension;
		string lastModified;
		int size;

	public:
		FileInfo(string fileName, string extension, string lastModified, int size); // construtor da classe. cria um descritor de arquivo
		bool setLastModified(string newlastModified);
		bool setSize(int newSize);

};

class DropboxClient {

	private:
		bool logged;
		struct sockaddr_in* devices[MAX_DEVICES];
		string uid;
		int activeSessions;
		FileInfo* files[MAX_NUM_FILES];
		
	public:
		DropboxClient(string newUid); // construtor da classe. cria um cliente e inicializa os atributos
		string getUid() { return uid; }
		int getActiveSessions() { return activeSessions; }
		struct sockaddr_in** getDevices() { return devices; }
		bool setLogged(bool logged);
		bool addDevice(struct sockaddr_in* addr);
		bool removeDevice(struct sockaddr_in* addr);
		bool addFile(FileInfo* fi);
		bool removeFile(string fileName);

};

#include <iostream>
#include <fstream>
#include <list>
#include "SocketManager.h"
#include "FileInformation.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>

using namespace std;

class DropperClient {
	
	private:
	
		string client_name;
		SocketManager* client_socket;
		list<FileInformation*> file_list; // lista de arquivos no diretório (hospedado no CLIENTE)
		bool error_flag;
	
	public:
	
		DropperClient() { error_flag = true; }; // construtor padrão. não utilizá-lo OK
		DropperClient(string client_name, string server_ip, int server_port); // OK
		bool bad() { return error_flag; } // OK
		bool connect(); // OK
		bool fillFileList();
		bool synchronize();
		bool sendFile(string file_path); // OK
		bool getFile(string file_name); // TODO Lógica reversa do send
		bool deleteFile(string file_path);
		bool closeSession(); // OK
	
};

#include <iostream>
#include <list>

#include "FileInformation.h"
#include "SocketManager.h"

class ClientData {
	
	private:
		
		string client_name;
		bool logged;
		list<FileInformation*> file_list; // lista de arquivos no diretório (hospedado no SERVIDOR)
		string devices[MAX_DEVICES]; // socket para comunicação com cada um dos dispositivos do cliente
		int sessionsCounter;
		
	public:
	
		ClientData();
		ClientData(string client_name);
		string getName() { return client_name; }
		bool isLogged() { return logged; }
		int getSessionsCounter() { return sessionsCounter; }
		bool addDevice(string device_address);
		bool removeDevice(string device_address);
		
};

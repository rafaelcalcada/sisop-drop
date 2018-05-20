#include <iostream>
#include "../include/DServer.h"

using namespace std;

int main()
{
	DServer* server = new DServer();
	if(server->bad()) {
		cout << "Houve um erro ao inicializar o servidor." << endl; }
	else {
		string cmd;
		cout << "Servidor inicializado com sucesso." << endl;
		cout << "Para desligar o servidor, digite 'quit' e pressione enter." << endl << endl;
		std::thread listening_thread(&DServer::listen, server);
		std::thread filelist_updater(&DServer::clientsFileListUpdaterDaemon, server);
		while(getline(cin,cmd)) {
			if(cmd == "quit") {
				listening_thread.detach();
				filelist_updater.detach();
				break; }
			if(cmd.substr(0,10) == "listclient") { 
				server->listClientFiles(cmd.substr(11));
				continue; }
			cout << "Comando inválido. Tente novamente." << endl; } }
	return 0;
}


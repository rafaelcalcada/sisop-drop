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
		while(getline(cin,cmd)) {
			if(cmd == "quit") {
				listening_thread.detach();
				break; }
			if(cmd.substr(0,10) == "listclient") { 
				if(cmd.size() >= 12) {
				server->listClientFiles(cmd.substr(11)); 
				continue; }
				else cout << "Comando inválido. Sintaxe: listclient [clientname]." << endl; continue; }
			cout << "Comando inválido. Tente novamente." << endl; } }
	return 0;
}


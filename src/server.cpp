#include <iostream>
#include "../include/DServer.h"

using namespace std;

int main()
{
	if(argc != 2) {
		cout << endl;
		cout << "Você chamou o programa da maneira errada." << endl;
		cout << "A forma correta é: ./server <nome-arq-conf>" << endl;
		cout << endl;
		return -1; }
	DServer* server = new DServer(string(argv[1]));
	if(server->bad()) {
		cout << "Houve um erro ao inicializar o servidor." << endl; }
	else {
		string cmd;
		cout << "Servidor inicializado com sucesso na porta " << SERVER_PORT << "." << endl;
		cout << "Para desligar o servidor, digite 'quit' e pressione enter." << endl << endl;
		std::thread coordination_thread(&DServer::coordinate, server);
		std::thread listening_thread(&DServer::listen, server);
		while(getline(cin,cmd)) {
			if(cmd == "") continue;
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


#include <iostream>
#include "../include/DServer.h"

using namespace std;

int main(int argc, char* argv[])
{
	DServer* server = new DServer();
	if(server->bad()) {
		cout << "Houve um erro ao inicializar o servidor." << endl; }
	else {
		if(argc > 1) {
			string arg1(argv[1]);
			string arg2(argv[2]);
			if(arg1 == "-sync") {
				server->setPrimary(string(argv[2]));
				bool primaryNotified = server->notifyPrimary();
				if(!primaryNotified) {
					cout << "Erro ao inicializar backup server no servidor primário." << endl;
					return -1; }
				else cout << "Servidor backup sincronizado e inicializado com sucesso." << endl; }
			std::thread listening_thread(&DServer::listen, server);
			std::thread sync_thread(&DServer::syncDaemon, server);
			string cmd;
			while(getline(cin,cmd)) {
				if(cmd == "") continue;
				if(cmd == "quit") {
					listening_thread.detach();
					sync_thread.detach();
					break; }
				if(cmd.substr(0,10) == "listclient") { 
					if(cmd.size() >= 12) {
					server->listClientFiles(cmd.substr(11)); 
					continue; }
					else cout << "Comando inválido. Sintaxe: listclient [clientname]." << endl; continue; }
				cout << "Comando inválido. Tente novamente." << endl; }	}
		else {
			string cmd;
			cout << "Servidor inicializado com sucesso. ID do servidor: " << server->getId() << "." << endl;
			cout << "Para desligar o servidor, digite 'quit' e pressione enter." << endl << endl;
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
				cout << "Comando inválido. Tente novamente." << endl; } } }
	return 0;
}


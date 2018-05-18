#include <iostream>
#include "../include/DClient.h"

using namespace std;

int main(int argc, char** argv)
{
	if(argc != 4) {
		cout << endl;
		cout << "Você chamou o programa da maneira errada." << endl;
		cout << "A forma correta é: ./client <nome-cliente> <ip-servidor> <porta-servidor>" << endl;
		cout << endl;
		return -1; }
	DClient* client = new DClient(string(argv[1]));
	if(client->bad()) cout << "DClient::DClient() - Erro ao inicializar o cliente." << endl;
	else {
		bool clientConnected = client->connect(argv[2], atoi(argv[3]));
		if(!clientConnected) {
			cout << "DClient::connect() - Erro ao conectar cliente." << endl;
			return -1; }
		cout << "Cliente conectado com sucesso ao servidor." << endl << endl;
		cout << "Digite um comando e pressione enter.\nPara obter a lista de comandos, digite 'help'." << endl << "> ";
		string cmd;
		while(getline(cin,cmd)) {
			if(cmd == "quit") {
				if(client->closeConnection()) cout << "Conexão com o servidor encerrada com sucesso." << endl;
				else cout << "DClient::closeConnection() - Problemas ao terminar conexão com o servidor." << endl;
				break; }
			if(cmd == "help") client->help();
			if(cmd.substr(0,6) == "upload") {
				string filePath = cmd.substr(7);
				bool fileSent = client->sendFile(filePath);
				if(fileSent) cout << "Upload realizado com sucesso." << endl << "> ";
				else cout << "Falha durante o upload. Tente novamente." << endl << "> "; } }
		return 0; }
}


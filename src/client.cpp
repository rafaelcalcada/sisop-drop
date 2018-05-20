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
		bool fileListFilled = client->fillFilesList("/sync_dir_" + client->getName());
		if(!fileListFilled) {
			cout << "DClient::fillFilesList - Erro ao pesquisar arquivos associados ao cliente." << endl;
			return -1; }
		bool clientConnected = client->connect(argv[2], atoi(argv[3]));
		if(!clientConnected) {
			cout << "DClient::connect() - Erro ao conectar cliente." << endl;
			return -1; }
		bool serverFilesListCreated = client->listServerFiles(DONT_PRINT);
		if(!serverFilesListCreated) {
			cout << "DClient::listServerFiles() - Erro ao obter lista de arquivos do cliente no servidor." << endl;
			return -1; }
		std::thread updater_daemon(&DClient::fileUpdaterDaemon, client);
		cout << "Cliente conectado com sucesso ao servidor." << endl << endl;
		cout << "Digite um comando e pressione enter.\nPara obter a lista de comandos, digite 'help'." << endl << "> ";
		string cmd;
		while(getline(cin,cmd)) {
			if(cmd == "quit") {
				updater_daemon.detach();
				if(client->closeConnection()) cout << endl << "Conexão com o servidor encerrada com sucesso." << endl << endl;
				else cout << "DClient::closeConnection() - Problemas ao terminar conexão com o servidor." << endl;
				break; }
			if(cmd == "help") client->help();
			if(cmd.substr(0,6) == "upload") {
				string filePath = cmd.substr(7);
				bool fileSent = client->sendFile(filePath);
				if(fileSent) cout << endl << "Upload realizado com sucesso." << endl << endl << "> ";
				else cout << endl << "Falha durante o upload. Tente novamente." << endl << endl << "> "; }
			if(cmd.substr(0,8) == "download") {
				string fileName = cmd.substr(9);
				bool fileReceived = client->receiveFile(fileName);
				if(fileReceived) cout << endl << "Download realizado com sucesso." << endl << endl << "> ";
				else cout << endl << "Falha durante o download. Tente novamente." << endl << endl << "> "; }
			if(cmd.substr(0,10) == "listclient") {
				cout << endl << "\e[1mArquivos neste dispositivo:\e[0m" << endl << endl;
				client->listFiles();
				cout << endl << "> "; }
			if(cmd.substr(0,10) == "listserver") {
				bool serverFilesListReceived = client->listServerFiles(PRINT); }
			if(cmd.substr(0,6) == "delete") {
				string filePath = cmd.substr(7);
				bool fileRemoved = client->deleteFile(filePath);
				if(fileRemoved) cout << endl << "Arquivo excluído com sucesso do cliente e do servidor." << endl << endl << "> ";
				else cout << endl << "Falha ao excluir arquivo. Tente novamente." << endl << endl << "> "; } }
		return 0; }
}


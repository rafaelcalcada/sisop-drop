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
		cout << endl;
		bool fileListFilled = client->fillFilesList("/sync_dir_" + client->getName());
		if(!fileListFilled) {
			cout << "DClient::fillFilesList - Erro ao pesquisar arquivos associados ao cliente." << endl;
			return -1; }
		bool clientConnected = client->connect(argv[2], atoi(argv[3]));
		if(!clientConnected) {
			cout << "DClient::connect() - Erro ao conectar cliente." << endl;
			return -1; }
		cout << "Cliente conectado com sucesso ao servidor." << endl << endl;
		bool serverFilesListCreated = client->listServerFiles(DONT_PRINT);
		if(!serverFilesListCreated) {
			cout << "DClient::listServerFiles() - Erro ao obter lista de arquivos do cliente no servidor." << endl;
			return -1; }
		client->synchronize();
		std::thread updater_daemon(&DClient::fileUpdaterDaemon, client);
		cout << "Digite um comando e pressione enter.\nPara obter a lista de comandos, digite 'help'." << endl;
		string cmd;
		while(getline(cin,cmd)) {
			if(cmd == "") continue;
			if(cmd == "quit") {
				updater_daemon.detach();
				if(client->closeConnection()) cout << "Conexão com o servidor encerrada com sucesso." << endl;
				else cout << "DClient::closeConnection() - Problemas ao terminar conexão com o servidor." << endl;
				break; }
			if(cmd == "help") {
				client->help();
				continue; }
			if(cmd.substr(0,6) == "upload") {
				string filePath = cmd.substr(7);
				client->getMutex()->lock();
				bool fileSent = client->sendFile(filePath);
				client->getMutex()->unlock();
				if(fileSent) cout << "Upload realizado com sucesso." << endl;
				else cout << "Falha durante o upload. Tente novamente." << endl;
				continue; }
			if(cmd.substr(0,8) == "download") {
				string fileName = cmd.substr(9);
				client->getMutex()->lock();
				bool fileReceived = client->receiveFile(fileName);
				client->getMutex()->unlock();
				if(fileReceived) cout <<"Download realizado com sucesso." << endl;
				else cout << "Falha durante o download. Tente novamente." << endl;
				continue; }
			if(cmd.substr(0,10) == "listclient") {
				cout << endl << "\e[1mArquivos neste dispositivo:\e[0m" << endl << endl;
				client->getMutex()->lock();
				client->listFiles();
				client->getMutex()->unlock();
				cout << endl;
				continue; }
			if(cmd.substr(0,10) == "listserver") {
				client->getMutex()->lock();
				bool serverFilesListReceived = client->listServerFiles(PRINT);
				client->getMutex()->unlock();
				continue; }
			if(cmd.substr(0,6) == "delete") {
				string filePath = cmd.substr(7);
				client->getMutex()->lock();
				bool fileRemoved = client->deleteFile(filePath);
				client->getMutex()->unlock();
				if(fileRemoved) cout << "Arquivo excluído com sucesso do cliente e do servidor." << endl;
				else cout << "Falha ao excluir arquivo. Tente novamente." << endl;
				continue; }
			cout << "Comando inválido. Tente novamente." << endl; }
		return 0; }
}


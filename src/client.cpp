#include <iostream>
#include "../include/DropperClient.h"

int main(int argc, char** argv) {
	
	if(argc != 4) {

		cout << endl;
		cout << "Você chamou o programa da maneira errada." << endl;
		cout << "A forma correta é: ./client <nome-cliente> <ip-servidor> <porta-servidor>" << endl;
		cout << endl;
		return -1;

	}

	DropperClient* client = new DropperClient(string(argv[1]), string(argv[2]), atoi(argv[3]));
	
	if(client->bad()) {

		cout << "Erro ao inicializar o cliente." << endl;

	} else {
	
		if(!client->connect()) {

			cout << "Conexão recusada pelo servidor." << endl;

		} else {

			cout << "Cliente inicializado com sucesso. Conexão aceita pelo servidor." << endl << endl;
			cout << "Digite um comando e pressione enter.\nPara obter a lista de comandos, digite 'help'." << endl << "> ";

			string cmd;

			while(getline(cin,cmd)) {

				if(cmd == "quit") {

					if(client->closeSession()) cout << "Conexão com o servidor encerrada com sucesso." << endl;
					else cout << "Problemas ao terminar conexão com o servidor." << endl;
					break;

				}
				if(cmd == "help")
				{
				 (client->help());
				}
				
				if(cmd.substr(0,6) == "upload") {
					
					string file_path = cmd.substr(7);
					if(client->sendFile(file_path)) cout << "Upload realizado com sucesso." << endl << "> ";
					else cout << "Falha durante o upload. Tente novamente." << endl << "> ";
					
				}
				
				if(cmd.substr(0,8) == "download") {
					
					string file_path = cmd.substr(9);
					if(client->getFile(file_path)) cout << "Download realizado com sucesso." << endl << "> ";
					else cout << "Falha durante o download. Tente novamente." << endl << "> ";
					
				}

			}

		}

		return 0;

	}

}

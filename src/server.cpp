#include <iostream>
#include <thread>

#include "../include/DropperServer.h"

using namespace std;

int main(int argc, char** argv) {

	DropperServer* server = new DropperServer();
	if(server->bad()) {
		
		cout << "Erro ao inicializar o servidor. Execução encerrada." << endl;
		return 0;

	} else {

		string cmd;
		cout << "Servidor inicializado com sucesso. Aguardando pedidos de conexão." << endl;
		cout << "Para desligar o servidor, digite 'quit' e pressione enter." << endl << endl;

		// dispara a thread que fica esperando conexões de clientes
		std::thread listening_thread(&DropperServer::listen, server);

		// espera comandos
		while(cin >> cmd) {

			if(cmd == "quit") {
				listening_thread.detach();
				break;
			}

			cout << "Comando inválido. Tente novamente." << endl;

		}

	}

}

#include <iostream>
#include "../include/DropboxServer.h"

int main(int argc, char* argv[]) {
	
	// cria uma instância do servidor e espera por pedidos de conexão de clientes
	DropboxServer* server = new DropboxServer();

	return 0;

}


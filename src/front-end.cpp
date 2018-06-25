#include <iostream>
#include "../include/DFrontEnd.h"

using namespace std;

int main(int argc, char** argv)
{
	if(argc != 2) {
		cout << endl;
		cout << "Você chamou o programa da maneira errada." << endl;
		cout << "A forma correta é: ./front-end <nome-arq-conf>" << endl;
		cout << endl;
		return -1; }
	DFrontEnd* frontEnd = new DFrontEnd(string(argv[1]));
	if(frontEnd->bad()) {
		cout << "Houve um erro ao inicializar o front-end." << endl; }
	else {
		string cmd;
		cout << "Front-end inicializado com sucesso na porta " << FRONT_END_CLIENT_FACED_PORT << "." << endl;
		cout << "Para ver o status da comunicação, digite  'status' e pressione enter." << endl << endl;
		cout << "Para desligar o front-end, digite 'quit' e pressione enter." << endl << endl;
		std::thread sockets_thread(&DFrontEnd::mainLoop, frontEnd);
		while(getline(cin,cmd)) {
			if(cmd == "") continue;
			if(cmd == "quit") {
				sockets_thread.detach();
				break; }
			if(cmd == "status") { 
				frontEnd->printStatus();
				continue; }
			cout << "Comando inválido. Tente novamente." << endl; } }
	return 0;
}

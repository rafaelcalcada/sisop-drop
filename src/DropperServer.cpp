#include "../include/DropperServer.h"

extern const int BUFFER_SIZE;

DropperServer::DropperServer() {

	this->server_socket = new SocketManager("localhost", SERVER_PORT_NUMBER);

	if(server_socket->bad()) error_flag = true;
	else {

		error_flag = false;
		const char *hmdr;
		if ((hmdr = getenv("HOME")) == NULL) {
			hmdr = getpwuid(getuid())->pw_dir;
		}
		homedir = string(hmdr);
		this->initialize();
		for(int i = 0; i < MAX_CONNECTIONS; i++) connection_id[i] = AVAILABLE;

	}

}

void DropperServer::listen() {

	socklen_t msgsz = sizeof(struct sockaddr_in);
	
	while(true) {

		char* msgbuf = new char[BUFFER_SIZE];

		// esta chamada é bloqueante
		if(!this->server_socket->receive(&msgbuf,&msgsz)) {
			cout << "Erro ao receber pedido de conexão de cliente." << endl;
			continue;
		}

		string request(msgbuf);

		// PROCESSAMENTO DO PEDIDO DE CONEXÃO

		if(request.substr(0, 7) == "connect") {

			string client_name = request.substr(8, request.size());
			ClientData* client = findClient(client_name);

			if(client == NULL) { // cliente sequer existe, logo não pode estar conectado. aceita conexão

				ClientData* new_client = new ClientData(client_name);
				string client_ip = this->server_socket->getSenderIP();
				int client_port = this->server_socket->getSenderPort();
				string client_address = client_ip + ":" + to_string(client_port);

				if(new_client->addDevice(client_ip)) {

					clients_list.push_back(new_client);
					string client_path = homedir + "/dropper-server/" + client_name;
					if(mkdir(client_path.c_str(), 0777) == 0) {

						if(acceptConnection(new_client, client_ip, client_port)) cout << "Nova conexão estabelecida: cliente '" << client_name << "', "
							<< client_address << endl;
						else cout << "Erro ao estabelecer conexão com o cliente." << endl;

					} else {
		
						cout << "Erro ao criar diretório do cliente. " << client_path << endl;
						string refused = "connection refused";
						this->server_socket->prepareToSend(client_ip, client_port);
						this->server_socket->send(refused.c_str(), refused.size()+1);

					}

				} else {
				
					cout << "Erro ao adicionar dispositivo do cliente. Dispositivo já possui conexão ativa." << endl;
					string refused = "connection refused";
					this->server_socket->prepareToSend(client_ip, client_port);
					this->server_socket->send(refused.c_str(), refused.size()+1);
				
				}

			} else {

				if(client->isLogged()) {
				
					if(client->getSessionsCounter() >= MAX_DEVICES) { // número de conexões excedido

						string refused = "connection refused";
						this->server_socket->prepareToSend(this->server_socket->getSenderIP(), this->server_socket->getSenderPort());
						this->server_socket->send(refused.c_str(), refused.size()+1);

					} else { // não excedido

						string client_ip = this->server_socket->getSenderIP();
						int client_port = this->server_socket->getSenderPort();
						string client_address = client_ip + ":" + to_string(client_port);

						if(client->addDevice(client_ip)) {

							if(acceptConnection(client, client_ip, client_port)) cout << "Nova conexão estabelecida: cliente '" << client_name << "', "
								<< client_address << endl;
							else {

								cout << "Erro ao estabelecer conexão com o cliente." << endl;
								string refused = "connection refused";
								this->server_socket->prepareToSend(client_ip, client_port);
								this->server_socket->send(refused.c_str(), refused.size()+1);

							}

						} else {

							cout << "Erro ao adicionar dispositivo do cliente. Dispositivo já possui conexão ativa." << endl;
							string refused = "connection refused";
							this->server_socket->prepareToSend(client_ip, client_port);
							this->server_socket->send(refused.c_str(), refused.size()+1);

						}

					}

				} else { // cliente não está logado. aceita conexão

					string client_ip = this->server_socket->getSenderIP();
					int client_port = this->server_socket->getSenderPort();
					string client_address = client_ip + ":" + to_string(client_port);

					if(client->addDevice(client_ip)) {

						if(acceptConnection(client, client_ip, client_port)) cout << "Nova conexão estabelecida: cliente '" << client_name << "', "
							<< client_address << endl;
						else {

							cout << "Erro ao estabelecer conexão com o cliente." << endl;
							string refused = "connection refused";
							this->server_socket->prepareToSend(client_ip, client_port);
							this->server_socket->send(refused.c_str(), refused.size()+1);

						}

					} else {

						cout << "Erro ao adicionar dispositivo do cliente. Dispositivo já possui conexão ativa." << endl;
						string refused = "connection refused";
						this->server_socket->prepareToSend(client_ip, client_port);
						this->server_socket->send(refused.c_str(), refused.size()+1);

					}

				}

			}

		}

		delete msgbuf;

	}

}

void DropperServer::messageProcessing(ClientData* client, SocketManager* client_socket, int conn_id) {

	socklen_t msgsz = sizeof(struct sockaddr_in);
	
	while(true) {

		char* msgbuf = new char[BUFFER_SIZE];
		
		if(!client_socket->receive(&msgbuf,&msgsz)) {
			cout << "Erro ao receber mensagem de cliente. (messageProcessing)" << endl;
			continue;
		}

		// PROCESSAMENTO DA MENSAGEM

		string msg(msgbuf);
		if(msg == "close connection") {

			if(client->removeDevice(client_socket->getSenderIP())) {
				
				connection_id[conn_id] = AVAILABLE;
				client_socket->send("close ok", 9);
				cout << "Conexão terminada pelo cliente (" << client->getName() << "). Dispositivo: " << client_socket->getSenderIP() << endl;
				client_socket->closeSocket();
				break;

			} else {
	
				client_socket->send("error", 6);

			}

		}
                        
		if(msg.substr(0,4) == "send") {
				
			// detecta o tamanho do arquivo e aloca memória para recebê-lo
			int file_size = atoi(msg.substr(msg.find_last_of(" ")+1).c_str());
			string file_name = msg.substr(5, msg.find_last_of(" ")-5);
			char* file_buf = new char[file_size];
			//cout << msg << endl; cout.flush();
			//cout << file_name << "." << file_size << endl; cout.flush();
			if(!client_socket->send("ackfile", 8)) {
				cout << "Erro ao enviar confirmação para recebimento de arquivo de cliente." << endl;
			} else {					
				// recebe o arquivo
				if(!client_socket->receiveFile(file_buf,file_size)) {
					cout << "Erro ao receber arquivo." << endl;
					delete file_buf;
				} else {
					
					string file_path = homedir + "/dropper-server/" + client->getName() + "/" + file_name;
					
					// cria o arquivo. sobrescreve conteúdo se arquivo já existia
					ofstream new_client_file;
					new_client_file.open(file_path.c_str(), ofstream::trunc | ofstream::binary);
					new_client_file.write(file_buf,file_size);
					new_client_file.close();
					
					if(!client_socket->send("complete", 9)) {
						cout << "Erro ao enviar confirmação de recebimento. Arquivo salvo." << endl;
					} else {
						cout << "Arquivo recebido com sucesso do cliente '" << client->getName() << "'." << endl;
					}
					
					delete file_buf;

					// TODO: adicionar arquivo na file_list do cliente
					
				}					
			}
				
				
		}
		
		if(msg.substr(0,7) == "receive") {
			
			// coleta informações sobre o arquivo sendo requisitado
			struct stat file_info;
			string file_path = homedir + "/dropper-server/" + client->getName() + "/" + msg.substr(8);
			if(stat(file_path.c_str(), &file_info) != 0) {
				cout << "Erro ao obter informações sobre o arquivo sendo enviado." << endl;
				client_socket->send("error", 6); // manda mensagem indicando erro
			} else {
				int file_size = (int) file_info.st_size;
				string response = "acksend " + to_string(file_size);				
				// informa o cliente sobre o tamanho do arquivo que será enviado
				if(!client_socket->send(response.c_str(), response.size()+1)) {
					cout << "Erro ao enviar informações sobre o arquivo a enviar." << endl;
				} else {
					// aguarda o recebimento da confirmação para envio
					memset(msgbuf,0,BUFFER_SIZE);
					if(!client_socket->receive(&msgbuf,&msgsz)) {
						cout << "Erro ao receber confirmação para envio ao cliente." << endl;
					} else {
						string confirm(msgbuf);
						if(confirm == "confirm") {
							// envia o arquivo para o cliente
							char* file_buf = new char[file_size];
							ifstream file;
							file.open(file_path.c_str(), ios_base::in);
							file.read(file_buf,file_size);
							file.close();
							if(!client_socket->send(file_buf, file_size)) {
								cout << "Erro ao enviar arquivo para o cliente." << endl;
							} else {
								// aguarda confirmação de recebimento do cliente
								memset(msgbuf,0,BUFFER_SIZE);
								if(!client_socket->receive(&msgbuf,&msgsz)) {
									cout << "Erro ao enviar arquivo. Confirmação de recebimento pelo cliente não recebida." << endl;
								} else {
									cout << "Arquivo enviado com sucesso para o cliente '" << client->getName() << "'." << endl;
								}
							}
						} else {
							cout << "Recebimento de arquivo abortado pelo cliente." << endl;
						}
					}
				}
			}			
		}
		
		delete msgbuf;

	}

}

bool DropperServer::acceptConnection(ClientData* client, string client_ip, int client_port) {

	int attempts = 0;

	while(attempts < 3) {

		int conn_id = this->getConnectionId();
			
		if(conn_id != -1) {

			SocketManager* client_socket = new SocketManager("localhost", SERVER_PORT_NUMBER + 1 + conn_id);

			if(client_socket->bad()) {

				this->connection_id[conn_id] = UNAVAILABLE;
				attempts++;

			} else {

				client_socket->prepareToSend(client_ip, client_port);
				this->connection_id[conn_id] = USED;
				
				thread* new_connection = new thread(&DropperServer::messageProcessing, this, client, client_socket, conn_id);
				string ack = "ack " + to_string(SERVER_PORT_NUMBER + 1 + conn_id);

				// envia um ack e termina tentativas
				client_socket->send(ack.c_str(), ack.size()+1);
				break;

			}

		} else {

			cout << "Impossível criar conexão com cliente. Servidor no limite de conexões simultâneas." << endl;
			return false;

		}

	}

	if(attempts == 3) return false;
	else return true;

}

ClientData* DropperServer::findClient(string client_name) {

	list<ClientData*>::iterator it;
	for(it = clients_list.begin(); it != clients_list.end(); ++it) {

		ClientData* client = *(it);
		if(client->getName() == client_name) return client;

	}

	return NULL;

}

bool DropperServer::initialize() {

	string server_path = homedir + "/dropper-server";

	struct stat st;
	if(stat(server_path.c_str(), &st) != -1) {

		DIR* dir = opendir(server_path.c_str());
		struct dirent* de;

		while((de = readdir(dir)) != NULL) {

			// ignora as entradas de diretório "." e ".."
			if(strcmp(de->d_name,".") == 0 || strcmp(de->d_name,"..") == 0) continue;

			struct stat destat;
			string entrypath = server_path + "/" + string(de->d_name);
			stat(entrypath.c_str(),&destat);

			if(S_ISDIR(destat.st_mode)) {

				string client_name(de->d_name);
				ClientData* new_client = new ClientData(client_name);
				clients_list.push_back(new_client);

			}

		}
	
		return true;

	} else {

		if(mkdir(server_path.c_str(), 0777) == 0) return true;
		else {
			cout << "Erro ao criar diretório do servidor." << endl;
			return false;
		}

	}

}

int DropperServer::getConnectionId() {
	
	for(int i = 0; i < MAX_CONNECTIONS; i++) {

		if(connection_id[i] == AVAILABLE) return i;

	}

	return -1;

}

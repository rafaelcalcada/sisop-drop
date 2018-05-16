#include "../include/DropperClient.h"

DropperClient::DropperClient(string client_name, string server_ip, int server_port) {

	this->client_name = client_name;
	this->error_flag = false;
	this->client_socket = new SocketManager();

	if(client_socket->bad()) {
		
		this->error_flag = true;
		cout << "Erro ao criar socket para o cliente." << endl;

	} else {

		if(!(client_socket->prepareToSend(server_ip, server_port))) {

			this->error_flag = true;
			cout << "Erro ao preparar socket do cliente para enviar ao servidor." << endl;

		}

	}

}
void DropperClient::help(){
  cout << "Ajuda:" << endl;
  cout << "help: comando de ajuda com todos comandos" << endl;
  cout << "download: faz o download de um arquivo no Dropbox. Sintaxe: nomedoarquivo /home/nomedousuario/pathtofile" << endl;
  cout << "upload: faz o upload de um arquivo do Dropbox. Sintaxe: nomedoarquivo /home/nomedousuario/pathtofile"<< endl;
  cout << "quit: sair do Dropbox." << endl;

}

bool DropperClient::connect() {

	string message = "connect " + this->client_name;

	int ok = this->client_socket->send(message.c_str(), message.size()+1);

	if(!ok) {

		cout << "Houve um erro ao tentar enviar solicitação de conexão." << endl;
		return false;

	} else {

		socklen_t msgsz = sizeof(struct sockaddr_in);
	
		char* msgbuf = new char[BUFFER_SIZE];

		bool ok = client_socket->receive(&msgbuf,&msgsz);
			
		if(!ok) cout << "Erro ao receber resposta do servidor." << endl;
		else {

			string response(msgbuf);
			if(response.substr(0,3) == "ack") {
				
				this->client_socket->changeDestinationPort(atoi(response.substr(4,response.size()).c_str()));
				return true;

			}
			if(response == "connection refused") return false;

		}
			
		delete msgbuf;

	}

}

bool DropperClient::closeSession() {

	string close_conn = "close connection";
	if(this->client_socket->send(close_conn.c_str(), close_conn.size()+1)) {
		
		socklen_t msgsz = sizeof(struct sockaddr_in);
		char* msgbuf = new char[BUFFER_SIZE];
		bool ok = client_socket->receive(&msgbuf,&msgsz);
		if(ok && string(msgbuf) == "close ok") { delete[] msgbuf; return this->client_socket->closeSocket(); }
		if(ok && string(msgbuf) == "error") { delete[] msgbuf; return false; }
		if(!ok) { delete[] msgbuf; return false; }

	} else return false;

}

bool DropperClient::sendFile(string file_path) {
	
	int msgsz = 0;
	
	ifstream file;
	file.open(file_path.c_str(), ifstream::in | ifstream::binary);
	
	if(file.is_open()) {
	
	    file.seekg(0, file.end);   
        int file_size = file.tellg();
        file.seekg(0, file.beg);
	    char* file_buf = new char[file_size];
	
        file.read(file_buf, file_size);
        file.close();
       
	    // solicita o envio do arquivo, indicando nome e tamanho
	    string message = "send " + file_path.substr(file_path.find_last_of("/")+1) + " " + to_string(file_size);
	    if(!client_socket->send(message.c_str(),message.size()+1)) {
		    cout << "Erro ao requisitar envio do arquivo." << endl;
		    return false;
	    }
	
	    // aguarda 'ackfile' do servidor
	    char* msgbuf = new char[BUFFER_SIZE];		
	    if(!client_socket->receive(&msgbuf,(socklen_t*)&msgsz)) {		
		    cout << "Erro ao receber resposta do servidor." << endl;
		    return false;
	    } else {
		    string response(msgbuf);
		    if(response == "ackfile") {

			    if(!client_socket->send(file_buf,file_size)) {
				    cout << "Erro ao enviar arquivo para o servidor." << endl;
				    return false;
			    } else {
				
				    // aguarda confirmação do servidor
				    char* msgbuf2 = new char[BUFFER_SIZE];
				    if(!client_socket->receive(&msgbuf2,(socklen_t*)&msgsz)) {
					
					    cout << "Erro ao receber confirmação de envio." << endl;
					    return false;
					
				    } else {
					
					    if(string(msgbuf2) == "complete") return true;
					    else return false;
					
				    }
				
			    }
			
		    } else {
			    cout << "Erro ao enviar arquivo. Confirmação do servidor não recebida." << endl;
			    return false;
		    }
	    }
    
    } else {
    
        cout << "Arquivo não encontrado." << endl;
        return false;
    
    }
	
}

bool DropperClient::getFile(string file_name) {
	
	int msgsz = 0;
		
	// solicita o recebimento do arquivo
	string message = "receive " + file_name;
	if(!client_socket->send(message.c_str(),message.size()+1)) {
		cout << "Erro ao requisitar recebimento do arquivo." << endl;
		return false;
	}
	
	// aguarda 'acksend' do servidor
	char* msgbuf = new char[BUFFER_SIZE];		
	if(!client_socket->receive(&msgbuf,(socklen_t*)&msgsz)) {		
		cout << "Erro ao receber acksend do servidor." << endl;
		delete msgbuf;
		return false;
	} else {
		string response(msgbuf);
		if(response.substr(0,7) == "acksend") {
			
			int file_size = atoi(response.substr(8).c_str());
			char* file_buf = new char[file_size];
			
			if(!client_socket->send("confirm", 8)) {
				cout << "Erro ao enviar confirmação para recebimento do servidor." << endl;
				delete file_buf;
				delete msgbuf;
				return false;
			} else {
				
				// aguarda recebimento do arquivo do servidor
				if(!client_socket->receive(&file_buf,(socklen_t*)&file_size)) {
					
					cout << "Erro ao receber confirmação de envio." << endl;
					delete msgbuf;
					delete file_buf;
					return false;
					
				} else {
					
					const char *homedir;
					if ((homedir = getenv("HOME")) == NULL) {
						homedir = getpwuid(getuid())->pw_dir;
					}
					string file_path = string(homedir) + "/sync_dir_" + this->client_name + "/" + file_name;
					ofstream new_client_file;
					new_client_file.open(file_path.c_str(), ios_base::trunc);;
					new_client_file << file_buf;
					new_client_file.close();
					
					// envia confirmação de recebimento para o servidor
					if(!client_socket->send("complete", 9)) {
						cout << "Erro ao enviar confirmação de recebimento para o servidor. Arquivo salvo." << endl;
						delete msgbuf;
						delete file_buf;
						return false;
					} else {
						delete file_buf;
						delete msgbuf;
						return true;
					}
					
				}
				
			}
			
		} else {
			cout << "Erro ao enviar arquivo. Confirmação do servidor não recebida ou recebida com erro." << endl;
			delete msgbuf;
			return false;
		}
	}	
	
}

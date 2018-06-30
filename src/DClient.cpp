#include "../include/DClient.h"

DClient::DClient(string clientName, bool bind)
{
	this->homeDir = string("/tmp");
	this->clientName = clientName;
	clientSocket = new DSocket();
	updateSocket = new DSocket();
	bool socketsOpen = clientSocket->isOpen();
	if(socketsOpen) 
		isWorking = true;
	if(bind) {
		bool upsockOpen = updateSocket->isOpen();
		bool binded = updateSocket->bindSocket("localhost", CLIENT_UPDATE_PORT);
		if(upsockOpen && binded) isWorking = true;
		else isWorking = false; }
}

bool DClient::createSyncDir(string basePath)
{
	string clientPath = homeDir + basePath + "/" + clientName;
	struct stat st;
	if(stat(clientPath.c_str(), &st) == -1) {
		bool clientDirCreated = (mkdir(clientPath.c_str(), 0777) == 0);
		if(!clientDirCreated) return false;
		else return true; }
	else return true;
}

void DClient::listen()
{
	DMessage* request = NULL;
	while(true) {
		bool requestReceived = updateSocket->receive(&request);
		if(!requestReceived) continue;
		if(request->toString().substr(0,10) == "newprimary") {
			string ip = string(inet_ntoa(updateSocket->getSenderIp()));
			cout << "Novo primário em: " << ip << endl;
			this->getMutex()->lock();
			bool connected = this->connect(ip.c_str(), SERVER_PORT);
			this->getMutex()->unlock();
			if(connected) cout << "Conectado ao novo primário com sucesso." << endl;
			else cout << "Falha ao conectar ao novo primário." << endl;
		}
	}
}

bool DClient::fillFilesList(string basePath)
{
	bool clientDirCreated = this->createSyncDir(basePath);
	if(!clientDirCreated) {
		cout << "DClient::createSyncDir() - Erro ao criar diretório para o cliente." << endl;
		return false; }
	string clientPath = homeDir + basePath + "/" + clientName;
	struct stat st;
	if(stat(clientPath.c_str(), &st) != -1) {
		DIR* dir = opendir(clientPath.c_str());
		struct dirent* de;
		while((de = readdir(dir)) != NULL) {
			if(strcmp(de->d_name,".") == 0 || strcmp(de->d_name,"..") == 0) continue;
			struct stat stfile;
			string entryPath = clientPath + "/" + string(de->d_name);
			stat(entryPath.c_str(),&stfile);
			if(S_ISREG(stfile.st_mode)) {
				DFile* newFile = new DFile(string(de->d_name), stfile);
				files.push_back(newFile); } } }
	return true;
}

void DClient::listFiles()
{
	list<DFile*>::iterator it;
	for(it = files.begin(); it != files.end(); it++) {
		DFile* file = *(it);
		cout << "\e[1m" << file->getName() << "\e[0m - " << file->printLastModified(); }
}

void DClient::help()
{
	cout << endl << "\e[1mCOMANDOS\e[0m" << endl << endl;
	cout << "\e[1mhelp\e[0m\nComando que ajuda com todos os comandos." << endl << endl;
	cout << "\e[1mdownload\e[0m\nFaz o download de um arquivo do servidor DBox.\nSintaxe: download [filename]" << endl << endl;
	cout << "\e[1mupload\e[0m\nFaz o upload de um arquivo para o servidor DBox.\nSintaxe: upload [pathtofile]" << endl << endl;
	cout << "\e[1mdelete\e[0m\nExclui um arquivo do servidor DBox.\nSintaxe: delete [filename]" << endl << endl;
	cout << "\e[1mlistclient\e[0m\nLista os arquivos do cliente no diretório local (sync_dir_[clientname])."<< endl << endl;
	cout << "\e[1mlistserver\e[0m\nLista os arquivos do cliente salvos no servidor." << endl << endl;
	cout << "\e[1mquit\n\e[0mEncerra a conexão com o servidor e fecha o DBox." << endl << endl;
}

bool DClient::isConnectedFrom(struct in_addr ipAddress)
{
	list<DSocket*>::iterator it;
	for(it = connections.begin(); it != connections.end(); ++it) {
		DSocket* conn = *(it);
		if(conn->getDestinationIp().s_addr == ipAddress.s_addr) return true; }
	return false;
}

bool DClient::connect(const char* serverAddress, int serverPort)
{
	bool destinationSetted = clientSocket->setDestination(serverAddress,serverPort);
	if(!destinationSetted) {
		cout << "DClient::connect() - Erro ao configurar endereço de destino do socket do cliente." << endl;
		return false; }
	DMessage* connectionRequest = new DMessage("connect " + clientName);
	bool requestSent = clientSocket->send(connectionRequest);
	if(!requestSent) {
		cout << "DClient::connect() - Erro ao enviar pedido de conexão." << endl;
		return false; }
	DMessage* serverReply = NULL;
	bool replyReceived = clientSocket->receive(&serverReply);
	if(!replyReceived) {
		cout << "DClient::connect() - Erro ao receber resposta do servidor." << endl;
		return false; }
	if(serverReply->toString() == "connection refused") {
		cout << "Conexão recusada pelo servidor." << endl;
		return false; }
	if(serverReply->toString().substr(0,3) == "ack") {
		int newServerPort = atoi(serverReply->toString().substr(4).c_str());
		bool serverPortChanged = clientSocket->setDestination(serverAddress,newServerPort);
		if(serverPortChanged) {
			//cout << "Conexão aceita pelo servidor na porta " << newServerPort << "." << endl;
			return true; } }
	return false;
}

bool DClient::sendFile(string basePath, string filePath, FileCopyOption option)
{
	ifstream file;
	file.open(filePath, ifstream::in | ifstream::binary);
	if(!file.is_open()) {
		cout << "Erro: arquivo não encontrado." << endl;
		return false; }
	file.seekg(0, file.end); 
	int fileSize = file.tellg();
	file.seekg(0, file.beg);
	struct stat fstat;
	bool statSuccess = (stat(filePath.c_str(),&fstat) == 0);
	if(!statSuccess) {
		cout << "DClient::sendFile() - Erro. Não foi possível obter informações sobre a data de modificação do arquivo." << endl;
		return false; }
	string fileName = filePath.substr(filePath.find_last_of("/")+1);
	DMessage* sendRequest = new DMessage("receive " + fileName + " " + to_string(fileSize));
	bool requestSent = clientSocket->send(sendRequest);
	if(!requestSent) {
		cout << "DClient::sendFile() - Erro. Requisição de envio não enviada." << endl;
		return false; }
	DMessage* serverReply = NULL;
	bool replyReceived = clientSocket->receive(&serverReply);
	if(!replyReceived) {
		cout << "DClient::sendFile() - Erro ao receber resposta da requisição." << endl;
		return false; }
	if(serverReply->toString() != "receive ack") {
		cout << "Erro. Servidor recusou recebimento do arquivo." << endl;
		return false; }
	int totalPackets = (int)((fileSize/BUFFER_SIZE)+1);
	int lastPacketSize = fileSize%BUFFER_SIZE;
	int packetsSent = 0;
	int filePointer = 0;
	int blockSize = BUFFER_SIZE;
	char* packetContent = new char[BUFFER_SIZE];
	while(packetsSent < totalPackets) {
		packetsSent++;
		file.seekg(filePointer, file.beg);
		filePointer += BUFFER_SIZE;
		if(packetsSent == totalPackets) blockSize = lastPacketSize;
		file.read(packetContent, blockSize);					
		DMessage* packet = new DMessage(packetContent,blockSize);
		bool packetSent = clientSocket->send(packet);
		if(!packetSent) {
			cout << "DClient::sendFile() - Erro ao enviar pacote para o servidor. Envio interrompido." << endl;
			return false; }
		DMessage* packetDeliveryStatus = NULL;
		bool packetDeliveryResponseReceived = clientSocket->receive(&packetDeliveryStatus);
		if(!packetDeliveryResponseReceived) {
			cout << "DClient::sendFile() - Erro. Confirmação de entrega de pacote não recebida. Envio interrompido." << endl;
			return false; }
		if(packetDeliveryStatus->toString() == "packet received") continue;
		else {
			cout << "DClient::sendFile() - Erro. Status de entrega de pacote desconhecido." << endl;
			return false; } }
	DMessage* lastModificationTime = new DMessage(to_string(fstat.st_mtim.tv_sec));
	bool lmtSent = clientSocket->send(lastModificationTime);
	if(!lmtSent) {
		cout << "DClient::sendFile() - Erro ao informar o servidor sobre a data da última modificação do arquivo." << endl;
		return false; }
	if(option == COPY) {
		ofstream fileCopy;
		string fileCopyPath = homeDir + basePath + "/" + clientName + "/" + fileName;
		fileCopy.open(fileCopyPath, ofstream::trunc | ofstream::binary);
		if(!fileCopy.is_open()) {
			cout << "DClient::sendFile() - Erro ao salvar cópia local no diretório do cliente." << endl;
			return false; }
		file.seekg(0, file.beg);
		fileCopy << file.rdbuf();
		fileCopy.close();
		file.close();
		struct utimbuf utb;
		utb.actime = fstat.st_mtim.tv_sec;
		utb.modtime = fstat.st_mtim.tv_sec;
		bool modTimeChanged = (utime(fileCopyPath.c_str(),&utb) == 0);
		if(!modTimeChanged) {
			cout << "DClient::sendFile() - Erro. Não foi possível modificar a data da última modificação e acesso." << endl;
			return false; } }
	bool fileListUpdated = this->updateFilesList(fileName, basePath + "/" + clientName);
	if(!fileListUpdated) {
		cout << "DClient::sendFile() - Erro. Lista de arquivos locais não atualizada." << endl;
		return false; }
	return true;
}

bool DClient::updateFilesList(string newFileName, string basePath)
{
	string newFilePath = homeDir + basePath + "/" + newFileName;
	struct stat stfile;
	bool statSuccess = (stat(newFilePath.c_str(), &stfile) != -1);
	if(!statSuccess) {
		cout << "DClient::updateFilesList() - Erro ao obter informações do arquivo." << endl;
		return false; }
	DFile* newFileInfo = new DFile(newFileName, stfile);
	list<DFile*>::iterator it;
	bool fileFound = false;
	for(it = files.begin(); it != files.end(); it++) {
		DFile* f = *(it);
		if(f->getName() == newFileName) { fileFound = true; break; } }
	if(fileFound) files.erase(it);
	files.push_back(newFileInfo);
	fileFound = false;
	for(it = serverFiles.begin(); it != serverFiles.end(); it++) {
		DFile* f = *(it);
		if(f->getName() == newFileName) { fileFound = true; break; } }
	if(fileFound) serverFiles.erase(it);
	serverFiles.push_back(newFileInfo);
	return true;
}

bool DClient::receiveFile(string basePath, string fileName)
{
	DMessage* receiveRequest = new DMessage("send " + fileName);
	bool requestSent = clientSocket->send(receiveRequest);
	if(!requestSent) {
		cout << "DClient::receiveFile() - Erro. Requisição de recebimento não enviada." << endl;
		return false; }
	DMessage* serverReply = NULL;
	bool replyReceived = clientSocket->receive(&serverReply);
	if(!replyReceived) {
		cout << "DClient::receiveFile() - Erro. Resposta do servidor não recebida." << endl;
		return false; }
	if(replyReceived && serverReply->toString() == "file not found") {
		cout << "Resposta do servidor: Arquivo não encontrado." << endl;
		return false; }
	if(replyReceived && serverReply->toString().substr(0,8) != "send ack") {
		cout << "DClient::receiveFile() - Erro. Resposta desconhecida." << endl;
		return false; }
	int fileSize = atoi(serverReply->toString().substr(9).c_str());
	string filePath = homeDir + basePath + "/" + clientName + "/" + fileName;
	ofstream newFile;
	newFile.open(filePath, ofstream::trunc | ofstream::binary);
	if(!newFile.is_open()) {
		cout << "DClient::receiveFile() - Erro ao receber arquivo. Não foi possível criar cópia local." << endl;
		return false; }
	DMessage* confirm = new DMessage("send ack ack");
	bool confirmationSent = clientSocket->send(confirm);
	if(!confirmationSent) {
		cout << "DClient::receiveFile() - Erro. Confirmação para recebimento não enviada." << endl;
		return false; }
	int totalPackets = (int)((fileSize/BUFFER_SIZE)+1);
	int packetsReceived = 0;
	int packetSize = 0;
	DMessage* packet = NULL;
	while(packetsReceived < totalPackets) {
		packetsReceived++;				
		bool packetReceived = clientSocket->receive(&packet);
		if(!packetReceived) {
			cout << "DClient::receiveFile() - Erro ao receber pacote do arquivo. Recebimento interrompido." << endl;
			newFile.close();
			return false; }
		DMessage* confirmation = new DMessage("packet received");
		bool confirmationSent = clientSocket->reply(confirmation);
		if(!confirmationSent) {
			cout << "DClient::receiveFile() - Erro ao enviar confirmação de recebimento de pacote de dados. Recebimento interrompido." << endl;
			newFile.close();
			return false; }
		newFile.write(packet->get(),packet->length()); }
	newFile.close();
	DMessage* lastModificationTime = NULL;
	bool lmtReceived = clientSocket->receive(&lastModificationTime);
	if(!lmtReceived) {
		cout << "DClient::receiveFile() - Erro. Data da última modificação do arquivo não recebida." << endl;
		return false; }
	time_t lmt = atol(lastModificationTime->toString().c_str());
	struct utimbuf utb;
	utb.actime = lmt;
	utb.modtime = lmt;
	bool modTimeChanged = (utime(filePath.c_str(),&utb) == 0);
	if(!modTimeChanged) {
		cout << "DClient::receiveFile() - Erro. Não foi possível modificar a data da última modificação e acesso." << endl;
		return false; }
	;
	bool fileListUpdated = this->updateFilesList(fileName, basePath + "/" + clientName);
	;
	if(!fileListUpdated) {
		cout << "DClient::receiveFile() - Erro. Lista de arquivos locais não atualizada." << endl;
		return false; }
	return true;
}

bool DClient::deleteLocalFile(string basePath, string fileName)
{
	string filePath = homeDir + basePath + "/" + clientName + "/" + fileName;
	struct stat fs;
	bool fileFound = (stat(filePath.c_str(), &fs) == 0);
	if(!fileFound) {
		cout << "DClient::deleteLocalFile() - O arquivo não existe no diretório local do cliente." << endl;
		return false; }
	bool fileRemoved = (remove(filePath.c_str()) == 0);
	if(!fileRemoved) {
		cout << "DClient::deleteLocalFile() - Erro ao excluir arquivo." << endl;
		return false; }
	list<DFile*>::iterator it;
	bool found = false;
	for(it = files.begin(); it != files.end(); it++) {
		DFile* file = *(it);
		if(file->getName() == fileName) { found = true; break; } }
	if(!found) {
		cout << "DClient::deleteLocalFile() - Erro ao excluir. Arquivo já não constava na lista do cliente." << endl;
		return false; }
	files.erase(it);
	return true;
}

bool DClient::deleteFile(string basePath, string fileName, DeleteLocalFileOption option)
{
	if(option == DELETE) {
		bool localFileRemoved = this->deleteLocalFile(basePath, fileName);
		if(!localFileRemoved) return false; }
	DMessage* deleteRequest = new DMessage("delete " + fileName);
	bool requestSent = clientSocket->send(deleteRequest);
	if(!requestSent) {
		cout << "DClient::deleteFile() - Erro. Requisição de exclusão não enviada." << endl;
		return false; }
	DMessage* serverReply = NULL;
	bool replyReceived = clientSocket->receive(&serverReply);
	if(!replyReceived) {
		cout << "DClient::deleteFile() - Erro. Resposta do servidor não recebida." << endl;
		return false; }
	if(replyReceived && serverReply->toString() == "file not found") {
		cout << "Resposta do servidor: Arquivo não encontrado." << endl;
		return false; }
	if(replyReceived && serverReply->toString() != "removed") {
		cout << "DClient::deleteFile() - Erro. Não foi possíve deletar o arquivo no servidor." << endl;
		return false; }
	list<DFile*>::iterator it;
	bool found = false;
	for(it = serverFiles.begin(); it != serverFiles.end(); it++) {
		DFile* file = *(it);
		if(file->getName() == fileName) { found = true; break; } }
	if(!found) {
		cout << "DClient::deleteFile() - Erro ao excluir. Arquivo já não constava na lista de arquivos do servidor." << endl;
		return false; }
	serverFiles.erase(it);
	return true;
}

bool DClient::listServerFiles(PrintOption printOnScreen)
{
	DMessage* request = new DMessage("list");
	bool requestSent = clientSocket->send(request);
	if(!requestSent) {
		cout << "DClient::listServerFiles() - Erro ao enviar requisição." << endl;
		return false; }
	DMessage* serverReply = NULL;
	bool replyReceived = clientSocket->receive(&serverReply);
	if(!replyReceived) {
		//cout << "DClient::listServerFiles() - Erro. Resposta do servidor não recebida." << endl;
		return false; }
	if(serverReply->toString() == "file list empty") {
		if(printOnScreen == PRINT) cout << "Resposta do servidor: diretório vazio." << endl;
		return true; }
	if(serverReply->toString().substr(0,9) != "list size") {
		cout << "DClient::listServerFiles() - Erro. Servidor mandou mensagem desconhecida." << endl;
		return false; }
	DMessage* confirm = new DMessage("confirm");
	bool confirmSent = clientSocket->send(confirm);
	if(!confirmSent) {
		cout << "DClient::listServerFiles() - Erro ao enviar confirmação para o servidor." << endl;
		return false; }
	int totalFiles = atoi(serverReply->toString().substr(10).c_str());
	int fileDataReceived = 0;
	serverFiles.clear();
	while(fileDataReceived < totalFiles) {
		fileDataReceived++;
		DMessage* fileData = NULL;
		bool fileDataReceived = clientSocket->receive(&fileData);
		if(!fileDataReceived) {
			cout << "DClient::listServerFiles() - Erro. Informações de arquivo não recebida." << endl;
			return false; }
		string fdstr = fileData->toString();
		string fileName = fdstr.substr(0,fdstr.find_last_of(" "));
		int fileSize = atoi(fdstr.substr(fdstr.find_last_of("[")+1,fdstr.find_last_of(",")).c_str());
		time_t lastModified = atol(fdstr.substr(fdstr.find_last_of(",")+1,fdstr.find_last_of("]")).c_str());
		DFile* newServerFile = new DFile(fileName, fileSize, lastModified);
		serverFiles.push_back(newServerFile);
		confirmSent = clientSocket->send(confirm);
		if(!confirmSent) {
			cout << "DClient::listServerFiles() - Erro ao enviar confirmação para o servidor." << endl;
			return false; } }
	if(printOnScreen == PRINT) {
		cout << endl << "\e[1mArquivos no servidor:\e[0m" << endl << endl;
		list<DFile*>::iterator it;
		for(it = serverFiles.begin(); it != serverFiles.end(); it++) {
			DFile* file = *(it);
			cout << "\e[1m" << file->getName() << "\e[0m - " << file->printLastModified(); }
		cout << endl; }
	return true;
}

bool DClient::closeConnection()
{
	bool closeRequestSent = clientSocket->send(new DMessage("close connection"));
	if(closeRequestSent) {		
		DMessage* serverReply = NULL;
		bool serverReplyReceived = clientSocket->receive(&serverReply);
		if(serverReplyReceived && serverReply->toString() == "connection closed") {
			bool clientSocketClosed = clientSocket->closeSocket();
			return clientSocketClosed; }
		else return false; }
	else return false;
}

DFile* DClient::findFile(list<DFile*>* fileList, string fileName)
{
	list<DFile*>::iterator it;
	for(it = fileList->begin(); it != fileList->end(); it++) {
		DFile* file = *(it);
		if(file->getName() == fileName) return file; }
	return NULL;
}

void DClient::autoUpload(string basePath)
{
	list<DFile*>::iterator it; int i = 0;
	list<DFile*> serverFilesCopy = serverFiles;
	list<DFile*> filesCopy = files;
	for(it = filesCopy.begin(); it != filesCopy.end(); it++) {
		DFile* cfile = *(it);
		DFile* sfile = findFile(&serverFiles,cfile->getName());
		if(sfile == NULL) {
			cout << endl << "Fazendo upload do arquivo (novo): " << cfile->getName() << "... ";
			this->getMutex()->lock();
			bool fileUploaded = this->sendFile(basePath, homeDir + basePath + "/" + clientName + "/" + cfile->getName(), DONT_COPY);
			this->getMutex()->unlock();
			if(!fileUploaded) cout << "Erro." << endl << endl; else cout << "Concluído." << endl << endl; }
		else {
			bool fileIsEqual = cfile->isEqual(sfile);
			if(!fileIsEqual) {
				cout << endl << "Fazendo upload do arquivo (modificado): " << cfile->getName() << "... ";
				this->getMutex()->lock();
				bool fileUploaded = this->sendFile(basePath, homeDir + basePath + "/" + clientName + "/" + cfile->getName(), DONT_COPY);
				this->getMutex()->unlock();
				if(!fileUploaded) cout << "Erro." << endl << endl; else cout << "Concluído." << endl << endl; } }
	}
	for(it = serverFilesCopy.begin(); it != serverFilesCopy.end(); it++) {
		DFile* sfile = *(it);
		DFile* cfile = findFile(&files,sfile->getName());
		if(cfile == NULL) {
			cout << endl << "Arquivo removido: " << sfile->getName() << ". Excluindo do servidor... ";
			this->getMutex()->lock();
			bool localFileRemoved = this->deleteFile(basePath, sfile->getName(), DONT_DELETE);
			this->getMutex()->unlock();
			if(!localFileRemoved) cout << "Erro." << endl << endl; else cout << "Concluído." << endl << endl; }
	}
}

void DClient::synchronize(string basePath, PrintOption option)
{
	list<DFile*>::iterator it; int i = 0;
	list<DFile*> serverFilesCopy = serverFiles;
	list<DFile*> filesCopy = files;
	for(it = serverFilesCopy.begin(); it != serverFilesCopy.end(); it++) {
		DFile* sfile = *(it);
		DFile* cfile = findFile(&files,sfile->getName());
		if(cfile == NULL) {
			if(option == PRINT) cout << endl << "Fazendo download do arquivo (novo): " << sfile->getName() << "... ";
			this->getMutex()->lock();
			bool fileDownloaded = this->receiveFile(basePath, sfile->getName());
			this->getMutex()->unlock();
			if(!fileDownloaded) if(option == PRINT) cout << "Erro." << endl << endl; else if(option == PRINT) cout << "Concluído." << endl << endl; }
		else {
			bool fileIsEqual = sfile->isEqual(cfile);
			if(!fileIsEqual) {
				if(option == PRINT) cout << endl << "Fazendo download do arquivo (modificado): " << sfile->getName() << "... ";
				this->getMutex()->lock();
				bool fileDownloaded = this->receiveFile(basePath, sfile->getName());
				this->getMutex()->unlock();
				if(!fileDownloaded) if(option == PRINT) cout << "Erro." << endl << endl; else if(option == PRINT) cout << "Concluído." << endl << endl; } }
	}
	for(it = filesCopy.begin(); it != filesCopy.end(); it++) {
		DFile* cfile = *(it);
		DFile* sfile = findFile(&serverFiles,cfile->getName());
		if(sfile == NULL) {
			if(option == PRINT) cout << endl << "Arquivo removido do servidor: " << cfile->getName() << ". Excluindo... ";
			this->getMutex()->lock();
			bool localFileRemoved = this->deleteLocalFile(basePath, cfile->getName());
			this->getMutex()->unlock();
			if(!localFileRemoved) if(option == PRINT) cout << "Erro." << endl << endl; else if(option == PRINT) cout << "Concluído." << endl << endl; }
	}
}

void DClient::updateReverse(string basePath)
{
	this->getMutex()->lock();
	files.clear();
	this->fillFilesList(basePath);
	this->getMutex()->unlock();
	this->autoUpload(basePath);
	this->getMutex()->lock();
	this->listServerFiles(DONT_PRINT);
	this->getMutex()->unlock();
	this->synchronize(basePath, DONT_PRINT);
}

void DClient::fileUpdaterDaemon(string basePath)
{
	while(true) {
		this_thread::sleep_for(chrono::seconds(5));
		this->getMutex()->lock();
		files.clear();
		this->fillFilesList(basePath);
		this->getMutex()->unlock();
		this->autoUpload(basePath);
		this->getMutex()->lock();
		this->listServerFiles(DONT_PRINT);
		this->getMutex()->unlock();
		this->synchronize(basePath, PRINT);
	}
}


#include "../include/DropboxClient.h"

DropboxClient::DropboxClient(string newUid) {

	uid = newUid;
	activeSessions = 0;
	logged = false;
	for(int i = 0; i < MAX_DEVICES; i++) devices[i] = NULL;
	for(int i = 0; i < MAX_NUM_FILES; i++) files[i] = NULL;
	
}

bool DropboxClient::addDevice(struct sockaddr_in* addr) {

	if(activeSessions < MAX_DEVICES) {

		int i;
		for(i = 0; i < MAX_DEVICES; i++) { // procura a primeira posição vazia do array
			if(devices[i] == NULL) break;
		}

		devices[i] = addr; // coloca nessa posição o endereço do dispositivo
		activeSessions++;

		return true;

	} else {

		return false;	

	}

}

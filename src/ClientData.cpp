#include "../include/ClientData.h"

extern const int MAX_DEVICES;

ClientData::ClientData() {

	client_name = "";
	logged = false;
	sessionsCounter = 0;
	for(int i = 0; i < MAX_DEVICES; i++) devices[i] = "";

}

ClientData::ClientData(string client_name) {

	this->client_name = client_name;
	this->logged = false;
	sessionsCounter = 0;
	for(int i = 0; i < MAX_DEVICES; i++) devices[i] = "";

}

bool ClientData::addDevice(string device_address) {

	if(sessionsCounter >= MAX_DEVICES) {
		
		cout << "Erro ao adicionar dispositivo. Número máximo de sessões excedido." << endl;
		return false;

	} else {

		for(int i = 0; i < MAX_DEVICES; i++) if(devices[i] == device_address) return false;

		for(int i = 0; i < MAX_DEVICES; i++) {

			if(devices[i] == "") {
	
				devices[i] = device_address;
				sessionsCounter++;
				logged = true;
				return true;

			}

		}

	}

	return false;

}

bool ClientData::removeDevice(string device_address) {

	if(sessionsCounter == 0) return true;

	for(int i = 0; i < MAX_DEVICES; i++) {

		if(devices[i] == device_address) {

			devices[i] = "";
			sessionsCounter--;
			if(sessionsCounter == 0) logged = false;
			return true;

		}

	}

	return false;

}

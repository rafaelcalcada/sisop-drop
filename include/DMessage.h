#ifndef DMESSAGE_H
#define DMESSAGE_H

#include <iostream>
#include <string>
#include <cstring>
using namespace std;

class DMessage {
	
private:
	char* message;
	int _length;
	
public:
	DMessage();
	DMessage(char* content, int length);
	DMessage(string content);
	string toString() { return string(message); }
	char* get() { return message; }
	bool set(char* content, int length);
	int length() { return _length; }
	
};

#endif

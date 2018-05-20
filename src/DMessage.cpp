#include "../include/DMessage.h"

DMessage::DMessage()
{
	this->message = NULL;
	this->_length = 0;
}

DMessage::DMessage(char* content, int length)
{
	this->message = content;
	this->_length = length;
}

DMessage::DMessage(string content)
{
	char* newMessage = new char[content.size()+1];
	strcpy(newMessage, content.c_str());
	this->message = newMessage;
	this->_length = (int) content.size()+1;
}

#ifndef DFILE_H
#define DFILE_H

#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
using namespace std;

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>

class DFile {
	
private:
	string _fileName;
	unsigned long _fileSize;
	time_t _lastModified;
	
public:
	DFile() { };
	DFile(string fileName, struct stat fileDescriptor);
	DFile(string fileName, unsigned long fileSize, time_t lastModified);
	void setLastModified(time_t newLastModifiedTime);
	void wasModifiedSince(time_t dateTime);
	string getName() { return _fileName; }
	unsigned long getSize() { return _fileSize; }
	time_t getLastModified() { return _lastModified; }
	string printLastModified() { return string(ctime(&_lastModified)); }
	bool isEqual(DFile* dfile);
	
};

#endif

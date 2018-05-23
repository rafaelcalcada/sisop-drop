#include "../include/DFile.h"

DFile::DFile(string fileName, struct stat fileDescriptor)
{
	_fileName = fileName;
	_fileSize = fileDescriptor.st_size;
	_lastModified = fileDescriptor.st_mtim.tv_sec;
}

DFile::DFile(string fileName, unsigned long fileSize, time_t lastModified)
{
	_fileName = fileName;
	_fileSize = fileSize;
	_lastModified = lastModified;
}

bool DFile::isEqual(DFile* dfile)
{
	if(_fileName != dfile->getName()) return false;
	if(_lastModified != dfile->getLastModified()) return false;
	return true;
}

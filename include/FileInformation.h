#include <iostream>
#include <string>
#include <list>

#include <sys/stat.h>
#include <dirent.h>

using namespace std;

class FileInformation {
	
	private:
	
		string name;
		string extension;
		string last_modified;
		int size;
		
	public:
	
		FileInformation();
		FileInformation(string path_name);
		string getName() { return name; }
		string getExtension() { return extension; }
		string getLastModified() { return last_modified; }
		int getSize() { return size; }
	
};

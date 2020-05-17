#include "MZLoaderHandle.h"
#include <fstream>

MZLoaderHandle::MZLoaderHandle()
{

}
MZLoaderHandle::MZLoaderHandle(std::string file)
{
	fileName = file;
 

		std::ifstream inFile(file, std::ifstream::ate | std::ifstream::binary);
		fileSize = inFile.tellg();

		std::cout << "opened " << file << "  " << fileSize << "\n";

}


MZLoaderHandle::~MZLoaderHandle()
{
}

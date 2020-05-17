#pragma once
#include "Structs.h"

// provides a handle (or pointer) for loading MZML file data
// to prevent using specific classes to store this
// if the loader changes (e.g., ti HDF5), and required changes can be reflected in this file


class MZLoaderHandle
{
public:
	MZLoaderHandle(std::string file);
	MZLoaderHandle();
	~MZLoaderHandle();
	inline std::string getFileName(){ return fileName; }
	int64 getFileSize() { return fileSize; }
private:
		enum loadType { none, loaded, failed, delayed };
		std::string fileName;
		int64 fileOffset;
		int64 fileSize;
		FileHandle fileHandle;
		loadType status;

};


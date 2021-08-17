#pragma once

 
#define USE_RAW_READER
 




#ifdef USE_RAW_READER
#define NOMINMAX

#include "MZLoader.h"
#include "MZLoaderHandle.h"

 
class RawLoader :
	public MZLoader
{
public:
	
	RawLoader(MZLoaderHandle filehandle, std::string path_to_executable) ;
	

	~RawLoader();

	MZData* loadDataPartial();
	MZData* loadData();
	size_t getFileSize();
	double getProgress();
	static void cleanUp();
	static inline const std::string rawReaderPath = "Readers/RawMS1Reader.exe";
	static inline const std::string rawReaderFolder = "Readers";


private:
	MZLoaderHandle fileHandle;
 
	std::string executable;



	 size_t size = 0;
	 size_t readScans = 0;
	 MZScan * lineCopy = NULL;
 
	 int status = 0;
};


#endif

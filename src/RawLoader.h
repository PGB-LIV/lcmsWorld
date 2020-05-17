#pragma once

#ifdef _WIN32
#define USE_RAW_READER
#endif




#ifdef USE_RAW_READER
#define NOMINMAX

#include "MZLoader.h"
#include "MZLoaderHandle.h"

#include "RawReader.h"
#include "Spectrum.h"

class RawLoader :
	public MZLoader
{
public:
	RawLoader(MZLoaderHandle filehandle);
	~RawLoader();

	MZData* loadDataPartial();
	MZData* loadData();
	size_t getFileSize();
	double getProgress();
	static void cleanUp();

private:
	MZLoaderHandle fileHandle;
	std::vector<MSToolkit::MSSpectrumType> filter;




	 size_t size = 0;
	 size_t readScans = 0;
	 MZScan * lineCopy = NULL;
	 MSToolkit::RAWReader rawHandle;
	 int status = 0;
};


#endif

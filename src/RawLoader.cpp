
#include "RawLoader.h"

#ifdef USE_RAW_READER

#include <iostream>
#include <fstream>
 


#include <iomanip>
#include <chrono>
#include <string_view>
#include "Error.h"
 
#define NOMINMAX
#include "RawReader.h"



#include "Spectrum.h"
#include "Globals.h"
 

size_t RawLoader::getFileSize()
{
	return fileHandle.getFileSize();
}

RawLoader::RawLoader(MZLoaderHandle handle)
{
	fileHandle = handle;

}


RawLoader::~RawLoader()
{
}






  

MZData* RawLoader::loadData()
{
	Range<mzFloat> mzRange;
	Range<lcFloat> lcRange;
	result = new MZData();

	MZData* value = loadDataPartial();

	result->setRange();
	result->setJagged(true);

	std::cout << "result = " << result << " level 1 lines = " << lcount << std::endl;


	return value;
}
 


double RawLoader::getProgress()
{
	if (size == 0)
	{
		return 0;
	}
	return (100.0 * readScans / size);
}




MZData* RawLoader::loadDataPartial()
{

	if (filter.size() == 0)
		filter.push_back(MSToolkit::MS1);

	if (rawHandle.getStatus() == false)
	{
		std::cout << "raw handle not opened \n";
		new Error(Error::ErrorType::file, "Raw files cannot be loaded.  \nYou should install Thermo's MSFileReader, Version 3.1 SP2\n(SP4 will not work)");
		return NULL;
	}
 
	// finished
	if (status == 2)
	{
		return NULL;
	}
	static std::chrono::system_clock::time_point start;


	try
	{
		std::string fileName = fileHandle.getFileName();

		//startup
		if (status == 0)
		{


			result = new MZData();

			start = std::chrono::system_clock::now();



			rawHandle.setMSLevelFilter(&filter);

			MSToolkit::Spectrum s;
			bool b = rawHandle.readRawFile(fileName.c_str(), s, 0);
			
			if (b == false)
			{
				new Error(Error::ErrorType::file, "raw file " + fileName + " could not be read");	
				return NULL;
			}
			size = rawHandle.getScanCount();

 

 			status = 1;
			rawHandle.backToStart();
			//get size and open
			
		}


		//still loading
		if (status == 1)
		{

			//might exit here if there are only ms1 spectra in file
			if (readScans >= size)
			{
 
				status = 2;
 				return NULL;

			}

			if (result->size() == 0)
			{

				// duplicating last scan to go over tile boundary
				//should havebeen copied
				if (lineCopy != NULL)
				{
					std::cout << " copy line across chunk boundary \n";
					result->append(lineCopy);
				}
				lineCopy = NULL;


			}

			//	loadPos = 0;


		
			while (readScans < size)
			{

				 

 
				int pos = rawHandle.getLastScanNumber();

					double progress = 100.0 * pos / size;
					Globals::statusText = "Loaded " + std::to_string((int)progress) + "%%";
					if (progress >= 100)
					{
						Globals::statusText = "Loaded 100%%, Processing data...";
					}
 
					MSToolkit::Spectrum s;
					bool b = rawHandle.readRawFile(fileName.c_str(), s, 0);
					//has finished
					if (b == false)
					{
						break;
					}
					if (b)
					{
						double lc = s.getRTime();
 
						if (lc > 0)
						{
							lcTime = (lcFloat) lc;

							auto data = s.getPeaks();
 
						 
							mzData.reserve(data->size());
							intensityData.reserve(data->size());

					 

							for (unsigned int i=0; i < data->size(); i++)
							{
								auto p = data->at(i);

						 
								mzData.push_back((mzFloat) p.mz);
								intensityData.push_back(p.intensity);

							}
							addScan();
							s.clearPeaks();
							s.clear();

						}

					}
					readScans++;


			 


				if (result->size() >= linesPerChunk)
				{
					std::cout << result->size() << " returning buffer \n";
					lineCopy = new MZScan(last_line);

					return result;
				}


			}

			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();

			std::cout << " load took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " \n";
			std::cout << "for parsing " << readScans << " ms1 spectra \n";

			Globals::statusText = fileName;

			status = 2;

			return result;

		}
	}
	catch (...)
	{
		// likely happens when shutting down and hence access memory which has been deleted
		return NULL;
	}
	//shouldn't happen - no status
	assert(0);
	return NULL;
}


#endif

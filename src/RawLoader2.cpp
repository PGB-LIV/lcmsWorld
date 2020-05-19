
#include "RawLoader.h"

#ifdef USE_RAW_READER

#include <iostream>
#include <fstream>
 


#include <iomanip>
#include <chrono>
#include <string_view>
#include "Error.h"
 
#define NOMINMAX
 
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



FILE* child;
char buffer[1024*1024];
MZData* RawLoader::loadDataPartial()
{

 
	// finished
	if (status == 2)
	{
		fclose(child);
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

			std::string exe = "d:/rawMS1/RawMS1Reader.exe " + fileName;
			child = _popen(exe.c_str(), "r");
 
			// error checking omitted.

	
			fgets(buffer, sizeof(buffer), child);
			//get the size
			buffer[strlen(buffer) - 1] = 0;
			size = atoi(buffer);

			std::cout << " init size " << size << "\n";
	
 

 

 			status = 1;
 
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

	 
				double progress = 100.0 * readScans / size;
				Globals::statusText = "Loaded " + std::to_string((int)progress) + "%%";
				if (progress >= 100)
				{
					Globals::statusText = "Loaded 100%%, Processing data...";
				}

				fgets(buffer, sizeof(buffer), child);
	
 
				lcTime = atof(buffer);
		 
				fgets(buffer, sizeof(buffer), child);
				int scanSize = atoi(buffer)  ;
				mzData.reserve(scanSize);
				intensityData.reserve(scanSize);

				fgets(buffer, sizeof(buffer), child);
				std::string mz(buffer);

				char* np = buffer;
				char* ptr = buffer;

 

				while (true)
				{
					char c = *ptr;
 
	 

					if (c == '\n' || c == ' ' || c == 0)
					
					{
						*ptr = 0;

						if (ptr == np)
							break;
							

						double val = atof(np);

 
						mzData.push_back((mzFloat)atof(np));
						np = ptr+1;
					}
					if (c == '\n' || c == 0)
						break;
					ptr++;

				}
				fgets(buffer, sizeof(buffer), child);
				np = buffer;
 
 
				ptr = buffer;
				while (true)
				{
					char c = *ptr;
					
 

					if (c == '\n' || c == ' ' || c == 0)
					{
						if (ptr == np)
							break;
						*ptr = 0;
						intensityData.push_back((mzFloat)atof(np));
						np = ptr+1;
					}
					if (c == '\n' || c == 0)
						break;
					ptr++;

				}
				
				assert(intensityData.size() == mzData.size());
				assert(intensityData.size() == scanSize);
  
					addScan();
 
 
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

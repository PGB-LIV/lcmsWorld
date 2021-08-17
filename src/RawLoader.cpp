// Using (e.g.) c# application to manage loading raw files

#include "RawLoader.h"


#include <iostream>
#include <fstream>


#include <cstdio> 
#include <iomanip>
#include <chrono>
#include <string_view>
#include "Error.h"

#define NOMINMAX

#include "Globals.h"
#ifdef _WIN32
#include <filesystem>
#endif


size_t RawLoader::getFileSize()
{
	return fileHandle.getFileSize();
}

RawLoader::RawLoader(MZLoaderHandle handle, const std::string path_to_executable)
{
	fileHandle = handle;
	executable = path_to_executable;
	
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


int curScan = 0;
int lastScan = 0;
FILE* child;
//maximum length of a line
char buffer[1024 * 1024 * 8];
MZData* RawLoader::loadDataPartial()
{

	if (Globals::closing)
		return NULL;


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

			std::string exe = executable;
			

#ifdef _WIN32
			exe = std::filesystem::absolute(exe.c_str()).string();
#endif
 
			std::ifstream f(exe.c_str());
			if (!f.good())
			{

				new Error(Error::ErrorType::file, executable+" was not found");
				return NULL;

			}



			


			try
			{
#ifdef _WIN32
				//no noise removal (file could be big!)
				//std::string cmd = "\"\""+ exe + "\" \"" + fileName +"\"\" -n";
				std::string cmd = "\"\"" + exe + "\" \"" + fileName + "\"\" ";
				std::cout << "starting " << cmd << " \n";

				child = _popen(cmd.c_str(), "r");
#else

				std::string cmd = "\"+exe + "\" \"" + fileName+"\"";
				child = popen(cmd.c_str(), "r");
#endif

			}
			catch (...)


			{

				new Error(Error::ErrorType::file, "lcmsWorld was not able to start the RawFileReader\nThis may require installing .Net Framework");
				return NULL;
			}



			
			//get the size



			try
			{
				fgets(buffer, sizeof(buffer), child);
				//ignore comments
				while (buffer[0] == '#')
					fgets(buffer, sizeof(buffer), child);


				size = atoi(buffer);

				std::cout << " init size " << size << "\n";
			}
			catch (...)
			{
#ifdef _WIN32
				new Error(Error::ErrorType::file, "lcmsWorld was not able to use the "+executable+"\nThis may require installing .Net Framework");
#else
				new Error(Error::ErrorType::file, "lcmsWorld was not able to use the " + executable + "\nThis may require installing Mono.");
#endif

				return NULL;
			}



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
			//this is the number of total scans, not the number of ms1 scans

			while (curScan < size)
			{
				if (Globals::closing)
					return NULL;

				double progress = 100.0 * curScan / size;
				Globals::statusText = "Loaded " + std::to_string((int)progress) + "%%";
				if (progress >= 100)
				{
					Globals::statusText = "Loaded 100%%, Processing data...";
				}

				fgets(buffer, sizeof(buffer), child);
				if (buffer[0] == '#')
					continue;

				curScan = atoi(buffer);
				if (curScan < 0 )

					break;

				if (curScan < lastScan)
				{

					std::cout << " Scans out of order " << curScan << " < " << lastScan << "\n";
				}

				lastScan = curScan;

				fgets(buffer, sizeof(buffer), child);
				lcTime = (lcFloat) atof(buffer);



				if (  lcTime < 0)

					break;

				fgets(buffer, sizeof(buffer), child);
				int scanSize = atoi(buffer);
				mzData.reserve(scanSize);
				intensityData.reserve(scanSize);

				fgets(buffer, sizeof(buffer), child);
				std::string mz(buffer);

				char* np = buffer;
				char* ptr = buffer;


				mzFloat last = 0;
				while (true)
				{
					char c = *ptr;



					if (c == '\n' || c == ' ' || c == 0)

					{
						*ptr = 0;

						if (ptr == np)
							break;


						double val = atof(np);

						if (val < last)
						{
							std::cout << " Error in m/z out of order " << val << " < " << last << "\n";
						}
						last = (mzFloat) val;
						mzData.push_back((mzFloat)val);
						np = ptr + 1;
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
						np = ptr + 1;
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

			if (lcTime < -1)
			{

				new Error(Error::ErrorType::file, "There was an error while loading file.");
				return NULL;

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




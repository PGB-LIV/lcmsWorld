#if 1
// Using c# application to manage loading raw files

#include "RawLoader.h"

#ifdef USE_RAW_READER

#include <iostream>
#include <fstream>


#include <cstdio> 
#include <iomanip>
#include <chrono>
#include <string_view>
#include "Error.h"

#define NOMINMAX

#include "Globals.h"
#include <filesystem>


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


int curScan = 0;
int lastScan = 0;
FILE* child;
char buffer[1024 * 1024];
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

			std::string exe = "RawReader/RawMS1Reader.exe";

			auto fullExe = std::filesystem::absolute(exe.c_str());

			std::string fe = fullExe.string();

			std::ifstream f(fe.c_str());
			if (!f.good())
			{

				new Error(Error::ErrorType::file, "RawMS1Reader.exe was not found");
				return NULL;

			}



			std::string cmd = fe + " " + fileName;

#ifdef _WIN32
			try
			{
				child = _popen(cmd.c_str(), "r");
			}
			catch (...)


			{

				new Error(Error::ErrorType::file, "lcmsWorld was not able to start the RawFileReader\nThis may require installing .Net Framework");
				return NULL;
			}

#else
			try
			{
				child = popen(cmd.c_str(), "r");
			}
			catch (...)


			{

				new Error(Error::ErrorType::file, "lcmsWorld was not able to start the RawFileReader\nThis may require installing Mono");				return NULL;
				return NULL;

			}
#endif



			
			//get the size



			try
			{
				fgets(buffer, sizeof(buffer), child);
				size = atoi(buffer);

				std::cout << " init size " << size << "\n";
			}
			catch (...)
			{
#ifdef _WIN32
				new Error(Error::ErrorType::file, "lcmsWorld was not able to use the RawFileReader\nThis may require installing .Net Framework");
#else
				new Error(Error::ErrorType::file, "lcmsWorld was not able to use the RawFileReader\nThis may require installing Mono.");
//				new Error(Error::ErrorType::file, "lcmsWorld was not able to use the RawFileReader\nThis may require installing Mono \n(e.g. sudo apt install mono-compliete.");				return NULL;

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


				double progress = 100.0 * curScan / size;
				Globals::statusText = "Loaded " + std::to_string((int)progress) + "%%";
				if (progress >= 100)
				{
					Globals::statusText = "Loaded 100%%, Processing data...";
				}

				fgets(buffer, sizeof(buffer), child);

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

				new Error(Error::ErrorType::file, "There was an error while loading .raw file.");
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


#endif












#else




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
						lcTime = (lcFloat)lc;

						auto data = s.getPeaks();


						mzData.reserve(data->size());
						intensityData.reserve(data->size());



						for (unsigned int i = 0; i < data->size(); i++)
						{
							auto p = data->at(i);


							mzData.push_back((mzFloat)p.mz);
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

#endif

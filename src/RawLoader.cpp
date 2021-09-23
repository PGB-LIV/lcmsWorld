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
long long total_size = 0;
int buffer_size = 1024 * 1024 * 16;
char* buffer = NULL;
double maxVal = 0;

MZData* RawLoader::loadDataPartial()
{

	if (buffer == NULL)
		buffer = (char*)std::malloc(buffer_size);
	if (Globals::closing)
		return NULL;

	// finished
	if (status == 2)
	{
		std::cout << "read points " << total_size << " \n";
		Globals::statusText = fileHandle.getFileName();

		fclose(child);
		std::free(buffer);
		buffer = NULL;
		return NULL;
	}
	static std::chrono::system_clock::time_point start;
	int max = 0;


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
				std::free(buffer);
				new Error(Error::ErrorType::file, executable + " was not found");
				buffer = NULL;
				return NULL;

			}






			try
			{
#ifdef _WIN32
				//no noise removal (file could be big!)
				std::string cmd = "\"\"" + exe + "\" \"" + fileName + "\"\" -n";
				
				if (Settings::noiseRemoval)
				{
					if (Settings::negativeNoiseRemoval)
						cmd = "\"\"" + exe + "\" \"" + fileName + "\"\" ";
					else
						if (Settings::noiseValue.size() > 1)
							cmd = "\"\"" + exe + "\" \"" + fileName + "\"\" -t "+Settings::noiseValue;

				}
				
				//				std::string cmd = "\"\"" + exe + "\" \"" + fileName + "\"\" ";
				 				//std::string cmd = "\"\"" + exe + "\" \"" + fileName + "\"\" -t 0.0005";

				std::cout << "starting " << cmd << " \n";


				child = _popen(cmd.c_str(), "r");
#else

				std::string cmd = "\"+exe + "\" \"" + fileName + "\"";
				child = popen(cmd.c_str(), "r");
#endif

			}
			catch (...)


			{
				std::free(buffer);
				buffer = NULL;
				new Error(Error::ErrorType::file, "The RawFileReader could not be started\nThis may require installing .Net Framework");
				return NULL;
			}




			//get the size



			try
			{
				fgets(buffer, buffer_size, child);
				//ignore comments
				while (buffer[0] == '#')
					fgets(buffer, buffer_size, child);


				size = atoi(buffer);

				std::cout << " init size " << size << "\n";
			}
			catch (...)
			{
#ifdef _WIN32
				new Error(Error::ErrorType::file, "tocmsWorld was not able to use the " + executable + "\nThis may require installing .Net Framework");
#else
				new Error(Error::ErrorType::file, "tocmsWorld was not able to use the " + executable + "\nThis may require installing Mono.");
#endif
				std::free(buffer);
				buffer = NULL;
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


				std::free(buffer);
				status = 2;
				buffer = NULL;
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
				{
					std::free(buffer);
					buffer = NULL;
					return NULL;
				}

				double progress = 100.0 * curScan / size;
				Globals::statusText = "Loaded " + std::to_string((int)progress) + "%%";
				if (progress > 99.5)
				{
					Globals::statusText = fileName;


				}

				fgets(buffer, buffer_size, child);
				if (buffer[0] == '#')
					continue;

				curScan = atoi(buffer);
				if (curScan < 0)

					break;

				if (curScan < lastScan)
				{

					std::cout << " Scans out of order " << curScan << " < " << lastScan << "\n";
				}

				lastScan = curScan;

				fgets(buffer, buffer_size, child);
				lcTime = (lcFloat)atof(buffer);



				if (lcTime < 0)

					break;


				//read scan length
				fgets(buffer, buffer_size, child);
				int scanSize = atoi(buffer);


				//check buffer is big enough - we know how many chars should be in a 64-bit float

				int max_line_length = scanSize * 25 + 4;
				if (buffer_size < max_line_length)
				{
					buffer_size = max_line_length;
					free(buffer);
					buffer = (char*)malloc(buffer_size);
				}

				mzData.reserve(scanSize);
				intensityData.reserve(scanSize);

				total_size += scanSize;

				//read the mz valuees
				fgets(buffer, buffer_size, child);

				//expand the buffer as needed
				//this should never be used, unless the file reader is giving unnecessarily long values
				//the strlen is a bit slow, would prefer to remove it, but it's only a couple of percent
				if (1)
					while (strlen(buffer) >= buffer_size - 1)
					{

						char* new_buffer = (char*)malloc(buffer_size * 2);
						strcpy(new_buffer, buffer);
						fgets(buffer, buffer_size, child);
						strcat(new_buffer, buffer);

						char* old = buffer;
						buffer = new_buffer;
						free(old);
						buffer_size = buffer_size * 2;

					}
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
						last = (mzFloat)val;
						mzData.push_back((mzFloat)val);
						np = ptr + 1;
					}
					if (c == '\n' || c == 0)
						break;
					ptr++;

				}

				//read the intensity valuees
				fgets(buffer, buffer_size, child);

				if (1)
					while (strlen(buffer) >= buffer_size - 1)
					{

						char* new_buffer = (char*)malloc(buffer_size * 2);
						strcpy(new_buffer, buffer);
						fgets(buffer, buffer_size, child);
						strcat(new_buffer, buffer);

						char* old = buffer;
						buffer = new_buffer;
						free(old);
						buffer_size = buffer_size * 2;

					}

				//		std::cout << " strlen " << strlen(buffer) << "  " << buffer_size << "\n";


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

						auto val = atof(np);

						if (val > maxVal)
						{


							maxVal = val;
						}
						//		val += 1;
						intensityData.push_back(val);
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



				//amend thhis to take into account size of lines

				int maxChunkSize = linesPerChunk;
				if (buffer_size > 100000)
					maxChunkSize /= 2;
				if (buffer_size > 250000)
					maxChunkSize /= 2;
				if (buffer_size > 500000)
					maxChunkSize /= 2;

				//now evenly distribute the chunks
				int sections = 1 + (size / maxChunkSize);

				maxChunkSize = 1 + (size / sections);
				//temp test - trying to check when it goes wrong
	 

				if (result->size() >= maxChunkSize)
				{
					std::cout << result->size() << " returning buffer \n";
					lineCopy = new MZScan(last_line);


					std::cout << "read  so far points " << total_size << " \n";

					return result;
				}


			}

			if (lcTime < -1)
			{
				std::free(buffer);
				buffer = NULL;
				new Error(Error::ErrorType::file, "There was an error while loading file.");
				return NULL;

			}


			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();

			std::cout << " load took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " \n";
			std::cout << "for parsing " << readScans << " ms1 spectra \n";

			std::cout << "loaded \n";
			Globals::statusText = fileName;
			
			std::cout << "read points " << total_size << " \n";

			status = 2;
			std::free(buffer);
			buffer = NULL;
			return result;

		}
	}
	catch (...)
	{
		if (buffer != NULL)
			free(buffer);

		buffer = NULL;
		// likely happens when shutting down and hence access memory which has been deleted
		return NULL;
	}
	//shouldn't happen - no status
	assert(0);
	return NULL;
}




#include "MzmlLoader2.h"
using std::string;
#include <iostream>
#include <fstream>
#include "Base64m.h"


#include <iomanip>
#include <chrono>
#include <string_view>
 #include "Zip.h"
#include "Globals.h"
#include "../mzParser/mzParser.h"

long long MzmlLoader2::getFileSize()
{
	return fileHandle.getFileSize();
}

MzmlLoader2::MzmlLoader2(MZLoaderHandle handle)
{
	fileHandle = handle;
	
}


MzmlLoader2::~MzmlLoader2()
{
 
}


 

MZData* MzmlLoader2::loadData()
{
 

	Range<mzFloat> mzRange;
	Range<lcFloat> lcRange;
	int getLod = 0;
	result = new MZData();

	std::cout << "start mzml load \n" << std::endl << std::flush;

	MZData* value = loadDataPartial();

	result->setRange();
	result->setJagged(true);

	std::cout << "result = " << result << " level 1 lines = " << lcount << std::endl;


	return value;
}

static int bufferSize = 16384 * 1024 * 2;
static std::vector<char> buffer1;

 

inline bool hasEnding(std::string_view const &fullString, std::string_view const &ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

static int status = 0;
static int linePos = 0;
static int lineCount = 0;
static int loadPos = 0;
static long long size = 0;
static long long readBytes = 0;
static MZScan * lineCopy = NULL;
static std::ifstream *filePtr;
static long long readPoints = 0;

double MzmlLoader2::getProgress()
{
	if (size == 0)
	{
		return 0;
	}
	return (100.0 * readBytes / size);
}

 
MZData* MzmlLoader2::loadDataPartial()
{


	
	static BasicSpectrum s;
	static BasicChromatogram chromat;
	static MzParser sax(&s, &chromat);
	
 
	std::cout << " mzml load \n" << status << std::flush;
	if (Globals::closing)
		return NULL;
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


			bool ok = sax.load((char*) fileName.c_str());
 
			if (!ok)
			{
				return NULL;
			}
			
			size = sax.highScan();
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
			//		std::cout << " copy line across chunk boundary \n";
					result->append(lineCopy);
				}
				lineCopy = NULL;


			}

			//	loadPos = 0;



			while (readScans < size)
			{


				if (Globals::closing)
					return NULL;

				int pos = readScans;
				double progress = 100.0 * pos / size;
				Globals::statusText = "Loaded " + std::to_string((int)progress) + "%%";
				if (progress >= 100)
				{
					Globals::statusText = "Loaded 100%%, Processing data...";
				}

		 
				bool b = (sax.readSpectrum(-1));


				while ((s.getMSLevel() != 1) && b)
				{
					readScans++;
 
					b = (sax.readSpectrum(-1));
				}


				//has finished
				if (b == false)
				{
					Globals::statusText = "Loaded 100%%, Processing data...";
					break;
				}
				if (b)
				{
 
					double lc = s.getRTime();

					if (lc > 0)
					{
						lcTime = (lcFloat)lc;

						int size = s.size();
 

						mzData.reserve(size);
						intensityData.reserve(size);


						readPoints += size;

						for (int i = 0; i < size; i++)
						{
			 


							mzData.push_back((mzFloat)s[i].mz);
							intensityData.push_back((signalFloat) s[i].intensity);

						}
						addScan();
 

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

			std::cout << readPoints << " data points\n";
			status = 2;

			return result;

		}
	}
	catch (...)
	{
		std::cout << " mzml return \n" << status << std::flush;

		// likely happens when shutting down and hence access memory which has been deleted
		return NULL;
	}

	std::cout << " mzml assert \n" << status << std::flush;
	//shouldn't happen - no status
	assert(0);
	return NULL;
}


 
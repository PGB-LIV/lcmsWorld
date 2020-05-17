#include "MzmlLoader.h"
using std::string;
#include <iostream>
#include <fstream>
#include "Base64m.h"

#include "Landscape.h"
#include "Builder.h"


#include <iomanip>
#include <chrono>
#include <string_view>
 #include "Zip.h"
#include "Globals.h"
#include "../mzParser/mzParser.h"

long long MzmlLoader::getFileSize()
{
	return fileHandle.getFileSize();
}

MzmlLoader::MzmlLoader(MZLoaderHandle handle)
{
	fileHandle = handle;
	
}


MzmlLoader::~MzmlLoader()
{
 
}



// will convert from float to double if required

std::vector<double> MzmlLoader::readBinary(std::string_view const & line)
{
	std::vector<double> empty;

	int startPos = (int) line.find("<binary>");

	if (startPos == string::npos)
		return empty;

	int endPos = (int) line.find("</binary>");
	if (endPos == string::npos)
		return empty;

	startPos += (int) strlen("<binary>");
	if (endPos >= startPos + 2)
	{

		std::vector<BYTE> decodedData = base64_decode_mem(line, startPos, endPos);
		if (compressedData > 0)
		{

			int length = Zip::UncompressData(&decodedData[0], (int)decodedData.size(), &zipBuffer[0], zipBufferSize);
			decodedData.assign(&zipBuffer[0], &zipBuffer[0] + length);
		}

		if (bigData64 == 0)
		{
			int length = (int) decodedData.size() / 4;
			float* floatData = (float*)&decodedData[0];
			std::vector<double> data(length);

			std::copy(floatData, floatData + length, &data[0]);

			return data;
		}

		size_t length = decodedData.size() / 8;
		double* floatData = (double*)&decodedData[0];


		std::vector<double> data(floatData, floatData + length);
		//for (int i = 0; i < length; i++)
		//	data[i] = ((double)floatData[i]);
		return data;



	}
	return empty;
}


// no conversion - return as a float (to prevent converting float to double to float)

std::vector<float> MzmlLoader::readBinaryFloat(const std::string_view  &line)
{
	std::vector<float> empty;

	int startPos = (int)line.find("<binary>");

	if (startPos == string::npos)
		return empty;

	int endPos = (int) line.find("</binary>");
	if (endPos == string::npos)
		return empty;

	startPos += (int) strlen("<binary>");
	if (endPos >= startPos + 2)
	{
		std::vector<BYTE> decodedData = base64_decode_mem(line, startPos, endPos);
		if (compressedData > 0)
		{

			int length = Zip::UncompressData(&decodedData[0], (int)decodedData.size(), &zipBuffer[0], zipBufferSize);
			//	std::cout << length << " ";

			decodedData.assign(&zipBuffer[0], &zipBuffer[0] + length);

			//	ret = deflate(&strm, flush);
		}

		/*
		if (bigData64) // converting from double to float - shouldn't be needed,as this function should only be called if it's already a float
		{
			size_t length = decodedData.size() / 8;
			double* floatData = (double*)&decodedData[0];

			std::vector<float> data(length);

			for (int i = 0; i < length; i++)
				data[i] = ((float)floatData[i]);

			return data;
		}
		*/

		size_t length = decodedData.size() / 4;
		float* floatData = (float*)&decodedData[0];
		std::vector<float> data(floatData, floatData + length);


		return data;




	}
	return empty;
}


std::vector<mzFloat> MzmlLoader::readBinaryMz(const std::string_view  &line)
{
	if (bigData64 == 0)
	{
		// if mzFloat is defined as a double, this should not compile...
		// (as an explicit conversion is needed)
		return readBinaryFloat(line);
	}

	auto doubles = readBinary(line);
	std::vector<mzFloat> empty;
	empty.reserve(doubles.size());
	for (auto i : doubles)
		empty.push_back((mzFloat)i);
	return empty;
}


std::vector<signalFloat> MzmlLoader::readBinarySignal(const std::string_view  &line)
{
	// assuming signalFloat is defined as double
	// if not, change to readBinaryFloat


	if (bigData64 == 0)
	{
		// if mzFloat is defined as a double, this should not compile...
		// (as an explicit conversion is needed)
		return readBinaryFloat(line);
	}

	auto doubles = readBinary(line);
	std::vector<mzFloat> empty;
	empty.reserve(doubles.size());
	for (auto i : doubles)
		empty.push_back((mzFloat)i);
	return empty;

}

 







bool  MzmlLoader::readMz(const std::string_view  &line)
{

	const std::vector<mzFloat> getmzData = readBinaryMz(line);
	if (getmzData.size() == 0)
		return false;

	mzData = getmzData;

	if (intensityData.size() == 0)
		return false;

	addScan();
	return true;
}





bool MzmlLoader::readIntensity(const std::string_view &line)
{
	const std::vector<signalFloat> getSignal = readBinarySignal(line);
	if (getSignal.size() == 0)
		return false;
	intensityData = getSignal;
	if (mzData.size() == 0)
		return false;

	addScan();
	return true;
}


 
//will use string_view atsome point

void MzmlLoader::processLine(const std::string_view & line)
{

	// std::cout << ":"<< line.substr(0, 32) << ";\n";
	size_t tagPos = line.find("<");
	if (tagPos == string::npos)
		return;

	std::string_view tag = line.substr(tagPos, 12);
	if (tag.rfind("<spectrum", 0) == 0)
	{
	//	found
	}

	if (tag.rfind("<cvParam", 0) == 0)
	{


		size_t namePos = line.find("name=\"", tagPos);
		if (namePos != string::npos)
		{

			std::string_view name = line.substr(namePos + 6, 32);

			size_t func = name.rfind("ms level\"");
				
				if (func != string::npos)
				{

					size_t func = line.find("value=\"1", 0);
					if (func != string::npos)
					{
						level = 1;
						return;
					}
					func = line.find("value=\"2", 0);
					if (func != string::npos)
					{
						level = 2;
						return;
					}

				}



 
			size_t firstArray = name.rfind("intensity array\"", 0);
			if (firstArray != string::npos)
			{
				getData = 1;
				return;
			}

			firstArray = name.rfind("m/z array\"");
			if (firstArray != string::npos)
			{
				getData = 2;
				return;
			}




			firstArray = name.rfind("time array\"", 0);
			if (firstArray != string::npos)
			{
				getData = 0;
				return;
			}

			firstArray = name.rfind("64-bit float", 0);
			if (firstArray != string::npos)
			{
				bigData64 = 1;
				return;
			}
			firstArray = name.rfind("32-bit float", 0);
			if (firstArray != string::npos)
			{
				bigData64 = 0;
				return;
			}
			firstArray = name.rfind("zlib compression", 0);
			if (firstArray != string::npos)
			{
				compressedData = 1;
				return;
			}


			firstArray = name.rfind("no compression", 0);
			if (firstArray != string::npos)
			{
				compressedData = 0;
				return;
			}
			firstArray = name.rfind("no compression", 0);
			if (firstArray != string::npos)
			{
				compressedData = 0;
				return;
			}
			
	

			//This is scan start time
			size_t first = line.find("accession=\"MS:1000016\"");
			if (first != string::npos)
			{
				size_t findUnit = line.find("unitAccession");
				if (findUnit != string::npos)
				{
					std::string_view rest = line.substr(findUnit + 7, line.size());

					if (rest.find("UO:0000031") != string::npos)
						useSeconds = false;

					if (rest.find("UO:0000010") != string::npos)
						useSeconds = true;

				}



				//should pull this out
				size_t foundValue = line.find("value=\"");
				if (foundValue != string::npos)
				{

					std::string_view number = line.substr(foundValue + 7, line.size());
					size_t end = number.find("\"");

					if (end == 0)
						std::cout << " 0?? \n" << line.size() << "\n"<<line << std::flush;

					if (end != string::npos)
						number = number.substr(0, end);

					
					lcTime = (lcFloat) stod(string(number));
					if (useSeconds)
						lcTime /= 60.0;

					index++;
				}
			}

			//	int startPos = line.find("<binary>");
		}
	}

	if (tag.rfind("<binary", 0) == 0)
	{


		if (index >= 0)
		{
			if (level == 1)
			{
				if (getData == 2)
				{
					if (readMz(line))
						getData = 0;

				}
				else if (getData == 1)
				{
					if (readIntensity(line))
						getData = 0;

				}

			}
		}
	}



}

 

MZData* MzmlLoader::loadData()
{
 
	Range<mzFloat> mzRange;
	Range<lcFloat> lcRange;
	int getLod = 0;
	result = new MZData();

	MZData* value = loadDataPartial();

	result->setRange();
	result->setJagged(true);

	std::cout << "result = " << result << " level 1 lines = " << lcount << std::endl;
	if (Builder::error != Builder::errorType::none)
		return NULL;
	

	return value;
}

int bufferSize = 16384 * 1024 * 2;
std::vector<char> buffer1;

 

inline bool hasEnding(std::string_view const &fullString, std::string_view const &ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

int status = 0;
int linePos = 0;
int lineCount = 0;
int loadPos = 0;
long long size = 0;
long long readBytes = 0;
MZScan * lineCopy = NULL;
std::ifstream *filePtr;

double MzmlLoader::getProgress()
{
	if (size == 0)
	{
		return 0;
	}
	return (100.0 * readBytes / size);
}

 
MZData* MzmlLoader::loadDataPartial()
{

	if (buffer1.size() == 0)
		buffer1.resize(bufferSize);
	if (zipBuffer.size() == 0)
		zipBuffer.resize(zipBufferSize);
 
	char* buffer = &buffer1[0];



	std::cout << " Load part " << status << " \n";
	
	// finished
	if (status == 2)
	{
		return NULL;
	}
	static std::chrono::system_clock::time_point start;


	try
	{
		//startup
		if (status == 0)
		{

			if (result != NULL)
			{
				std::cout << "delete old result? " << result->size() << "\n";
				delete result;
			}

			result = new MZData();

			start = std::chrono::system_clock::now();

			string fileName = fileHandle.getFileName();
			filePtr = new std::ifstream(fileName, std::ios::binary | std::ios::ate);
			size = filePtr->tellg();

			readBytes = 0;
			filePtr->seekg(0, std::ios::beg);
			status = 1;
		}


		//still loading
		if (status == 1)
		{

			if (readBytes >= size)
			{
				std::chrono::system_clock::time_point end = std::chrono::system_clock::now();

				std::cout << " load took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " \n";
				std::cout << "for parsing " << lineCount << " lines \n";


				status = 2;
				filePtr->close();
				delete(filePtr);
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


			char inquote = 0;
			while (readBytes < size)
			{


				int readSize = bufferSize - loadPos;
				if (readBytes + readSize > size)
					readSize = (int)(size - readBytes);

			

				if (filePtr->read(buffer + loadPos, readSize))
				{
					linePos = 0;
					std::cout << "read chunk " << readSize << " " << lineCount << "  " << loadPos << "\n";
					readBytes += readSize;
					readSize += loadPos;


					double progress = 100.0 * readBytes / size;
					Globals::statusText = "Loaded " + std::to_string((int)progress) + "%%";
					if (progress >= 100)
					{
						Globals::statusText = "Loaded 100%%, Processing data...";
					}

					for (int i = 0; i < readSize; i++)

					{


						char ch = buffer[i];



						if (inquote == 0 && (ch == '>'))
						{




							std::string_view line(&buffer[linePos], i - linePos + 1);



							if (!hasEnding(line, "<binary>"))
							{
								processLine(line);
								lineCount++;
								linePos = i + 1;
							}

						}
					}
					loadPos = readSize - linePos;

					std::memcpy(&buffer[0], &buffer[linePos], loadPos);



				}


				if (result->size() >= linesPerChunk)
				{
					std::cout << result->size() << " returning buffer \n";
					lineCopy = new MZScan(last_line);
 
					return result;
				}


			}
			string fileName = fileHandle.getFileName();

			Globals::statusText = fileName;

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


 
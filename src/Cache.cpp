#include "gl3w/gl3w.h"    // Initialize with gl3wInit()

#include <array>
#include <iostream>
#include <thread>
#include <random>
#include <time.h>
#include <thread>
#include <fstream>
#include <atomic>
#include "shader.hpp"
#include "texture.hpp"
#include "Structs.h"
#include "Mesh.h"
#include "Landscape.h"
#include "Tile.h"
#include "GLMesh.h"
#include "MZLoader.h"
#include "MzmlLoader.h"
#include "RawLoader.h"
 #include "Utils.h"
#include <sstream>
#include <iomanip>
// #include <GL/gl3w.h>    
#include <cmath>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp" // after <glm/glm.hpp>

#include "SampleLoader.h"
#include "Camera.h"
#include "Error.h"
#include "Zip.h"
#include "LCHttp.h"
#include "Annotations.h"
#include "Cache.h"
#include "Builder.h"
#include "Render.h"
bool Cache::makeMetaFile = false;

int Cache::queueSize = 0;

std::fstream Cache::metaFile;

std::string sanity_string = "lcms.file.0x34012c";

std::streamoff  Cache::cachePosition = 0;
std::string Cache::cacheFileEnding = ".lcms";
std::fstream Cache::cacheFile;
std::streamoff  Cache::cacheFileSize = 0;
bool Cache::cacheCacheFile = false;
bool Cache::loadFromURL = false;
bool Cache::compressData = true;
std::mutex Cache::meshLock;
std::mutex Cache::cacheLock;

std::vector<byte> Cache::dataCache;

bool Cache::processQueue = true;
std::mutex Cache::loadmtx;
std::condition_variable Cache::loadcv;
std::vector<Tile*> Cache::loadQueue;
std::mutex Cache::loadQueueLock;
int Cache::make_tiles = 0;
int Cache::made_tiles = 0;


std::mutex Cache::makeQueueLock;
std::condition_variable Cache::makecv;


std::queue<Tile*> Cache::makeQueue;

bool Cache::cancelling = false;
bool Cache::loadQueueEnded = false;
bool Cache::makeQueueEnded = false;


void Cache::insertHeaderSpace(std::fstream &outFile)
{
	std::vector<char> empty(cacheHeaderSize);
	for (int i = 0; i < cacheHeaderSize; i++)
		empty[i] = -1;
	outFile.write(&empty[0], empty.size());
}



void Cache::setupCache(std::string inputFileName, bool newFile)
{
	std::string fileName = inputFileName;
	if (newFile)
		fileName = inputFileName + cacheFileEnding;

	std::cout << " load cache " << inputFileName << "\n";

	for (int i = 1; i < 10; i++)
	{
		auto flags = std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc;
		if (newFile == false)
			flags = std::ios::in | std::ios::binary;
		fileName = convertFilename(fileName);


		cacheFile.open(fileName, flags);
		if (cacheFile.is_open())
		{
			std::cout << "Cache file opened " << fileName << "\n";
			cacheFile.seekp(0);
			cachePosition = 0;
			if (newFile)
			{
				//write some space at the beginning,  which will be used for some headers
				cachePosition = cacheHeaderSize;
				insertHeaderSpace(cacheFile);


			}
			return;
		}
		fileName = inputFileName + "_" + std::to_string(i) + cacheFileEnding;
	}
	std::cout << "Error - cache file could not be opened " << fileName << " \n";
	new Error(Error::ErrorType::file, "lcms file " + fileName + " could not be opened");

}



inline std::vector<byte> Cache::getDataFromFile(DataSource d)
{
	std::vector<byte> empty;


#if 1
	if (loadFromURL)
		return LCHttp::getFromHttp(d);
#endif
	std::lock_guard<std::mutex> lock(cacheLock);


 

	cacheFile.seekg(d.offset);
	std::vector<byte> data(d.compressed_size);
	cacheFile.read((char*)&data[0], d.compressed_size);
	auto readSize = cacheFile.gcount();

	if (readSize != d.compressed_size)
	{
		std::cout << "file read size mismatch \n";
 
		new Error(Error::ErrorType::file, "There was a problem loading data from the .lcms file. \nIit may be invalid, or the disk may be full.\n");

		return empty;

	}

	if (d.compressed_size != d.size)
	{
		std::vector<byte> decompressedData(d.size);

		int length = Zip::UncompressData(&data[0], (int)data.size(), &decompressedData[0], decompressedData.size());
		data = decompressedData;
		//decompress data 
	}


 


	return data;
}

void Cache::loadCache()
{
	//wont work on 32 but, but not called anyway
	cacheLock.lock();
	cacheFile.seekg(0, std::ios::end);
	cacheFileSize = cacheFile.tellg();
	cacheFile.seekg(0, std::ios::beg);
	dataCache.resize(cacheFileSize);
	cacheFile.read((char*)&dataCache[0], cacheFileSize);
	cacheLock.unlock();
	std::cout << " cache loaded \n";
}

std::vector<byte> Cache::getData(DataSource d)
{
	if (cacheCacheFile)
	{

		if (dataCache.size() == 0)
		{
			if (loadQueue.size() == 0)
			{
				const auto data = getDataFromFile(d);
				std::thread t1(&loadCache);
				t1.detach();


				return data;
			}
			else
				return getDataFromFile(d);
		}

		cacheFile.seekg(d.offset);
		std::vector<byte> data(d.size);


		if (d.compressed_size != d.size)
		{


			long long length = Zip::UncompressData(&dataCache[d.offset], (int)d.compressed_size, &data[0], (int) d.size);
			assert(length == d.size);
			//decompress data 
		}
		else
		{
			memcpy(&data[0], &dataCache[d.offset], d.size);
		}


		return data;

	}
	return getDataFromFile(d);
}

//note - can Compress the data here (for e.g. internet transfer)
//This could throw an error - e.g., out of disk space

// use ZLIB compression on data

DataSource Cache::putData(const std::vector<byte> & data)
{
	cacheLock.lock();

	cacheFile.seekp(cachePosition);
	size_t  size = data.size();
	size_t comp_size = size;
	if (makeMetaFile)
	{
		// if I am making a metafile, I am going to compress this data
		// if it doesn't compress, I just write it as-is
		// the size == compressed size indicates that it is not compressed
		int bufferSize = Zip::GetMaxCompressedLen(data.size());
		std::vector<byte> compressed_buffer;
		compressed_buffer.resize(bufferSize);
		comp_size = Zip::CompressData(&data[0], data.size(), &compressed_buffer[0], bufferSize);
		if (comp_size < size)
		{
			cacheFile.write((char*)&compressed_buffer[0], comp_size);
		}
		else
		{
			comp_size = size;
			cacheFile.write((char*)&data[0], size);
		}

		//	std::cout << " was " << data.size() << " now " << len << "  " << (len*100.0 / data.size()) << "\n";
	}
	else
	{
		if (cacheFile.write((char*)&data[0], size))
		{

		}
		else
		{		 std::cout << " error writing " << size << " to cache "    << "\n";

			 new Error(Error::ErrorType::file, "Could not write to disk.  ");
		}
	}
	 	DataSource d;
	d.sourceIndex = 0;
	d.offset = cachePosition;
	d.size = size;
	d.compressed_size = comp_size;

	cachePosition += comp_size;
	cacheLock.unlock();
	return d;
}




void Cache::processLoadQueue()
{
	std::unique_lock<std::mutex> lck(loadmtx);
	loadQueueEnded = false;

	while (processQueue)
	{
		if (loadQueue.empty())
		{
			queueSize = 0;
			//wait
			loadcv.wait(lck);
		}
		else
		{
		 
			loadQueueLock.lock();
			

			// just store the values so that they don't change during sort
			for (auto t : loadQueue)
				t->storeLocation();

			queueSize = loadQueue.size();
			std::sort(loadQueue.begin(), loadQueue.end(), Tile::compareTilePtr);


			Tile* tile = loadQueue.back();
			loadQueue.pop_back();

			loadQueueLock.unlock();
 #if 1

			// remove tiles from queue if they are no longer needed 
			//make sure that this is smaller then the amount needed to reload them....
	
//			tile->setScreenSize();


			if ((tile->getScreenSize() <= 0.003) && (tile->LOD > 1) )
			{
				if (tile->drawStatus == DrawStatus::loadingData)
				{

					tile->drawStatus = DrawStatus::noData;
				}

			}
			else
#endif
			{
				DataSource d = tile->getSource();
				if (d.size > 0)
				{
					auto data = getData(d);

					if (data.size() != d.size)
					{
						//we had an error, so stick it back in the queue
						//if it's an http error, there should be some delay
						std::cout << " size mismatch " << data.size() << " , " << d.size << "\n";

						loadQueueLock.lock();
						loadQueue.push_back(tile);
						loadQueueLock.unlock();
					}
					else

						tile->deSerialiseData(data);
					// data.clear();
				}
			}

		}

	}
	loadQueueEnded = true;

}

void Cache::loadDataFromCache(Tile* tile)
{


	loadQueueLock.lock();

	loadQueue.push_back(tile);
	loadQueueLock.unlock();
	// notify loadqueue lock 
	  loadcv.notify_all();
	return;
}


void Cache::closeCache()
{
	// At the moment, only called when closing program
	// hence just need to stop threads and close file
	// In future, will need to close down nicely
	
	processQueue = false;

	int count = 0;
	while (!Cache::loadQueueEnded && !Cache::makeQueueEnded)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		// 1 second is more than long enough
		if (++count > 100)
		{
			std::cout << "Not waiting for cache queue to end \n";
			break;
		}
	}


 
	std::queue<Tile *>().swap(makeQueue);
	loadQueue.clear();
	cachePosition = 0;
	cacheFile.close();
}

void Cache::processmakeQueueFG()
{


	makeQueueLock.lock();
	while (!makeQueue.empty())
	{
	
		Tile* tile = makeQueue.front();

		makeQueue.pop();


		tile->owner->makeMeshProcess(tile);
	}
	makeQueueLock.unlock();
 
}


void Cache::processmakeQueue()
{
	makeQueueEnded = false;

	while (processQueue)
	{
		if (makeQueue.empty())
		{
			//should really wait until gets notified
			std::this_thread::sleep_for(std::chrono::milliseconds(10));


		}
		else
		{
			makeQueueLock.lock();
			Tile* tile = makeQueue.front();
 
			makeQueue.pop();
			makeQueueLock.unlock();

			tile->owner->makeMeshProcess(tile);


		}

	}

	makeQueueEnded = true;
}



void Cache::makeMeshStandard(Tile* tile)
{
	std::lock_guard<std::mutex> lock(makeQueueLock);

  
	makeQueue.push(tile);
 
	// should notify loadqueue lock
 
}





void Cache::saveMetaData(Landscape *l, std::string inputFileName, std::string url)
{
	// l->addTile(tile);

	std::string fileName = inputFileName + cacheFileEnding;
	auto flags = std::ios::in | std::ios::out | std::ios::binary;
	fileName = convertFilename(fileName);
	metaFile.open(fileName, flags);
	if (metaFile.is_open() == false)

	{
		new Error(Error::ErrorType::file, "File " + fileName + " could not be opened");

		std::cout << "Error - meta file could not be opened " << fileName << "\n";
		return;
	}

	metaFile.seekg(0, std::ios::end);

	auto position = metaFile.tellg();
	std::cout << " write at position " << position << "\n";


	metaFile.seekp(position, std::ios::beg);
	auto data = l->serialiseData();
	size_t  size = data.size();
	size_t  compressedSize = size;
	if (url.length() > 1)
	{
		int bufferSize = Zip::GetMaxCompressedLen(data.size());
		std::vector<byte> compressed_buffer;
		compressed_buffer.resize(bufferSize);
		compressedSize = Zip::CompressData(&data[0], data.size(), &compressed_buffer[0], bufferSize);
		std::cout << "compressedSize " << compressedSize << "\n";
		if (compressedSize < size)
		{
			metaFile.write((char*)&compressed_buffer[0], compressedSize);
		}
		else
		{
			compressedSize = size;
			metaFile.write((char*)&data[0], size);
		}

	}
	else
	{

		std::cout << "size " << size << "\n";
		metaFile.write((char*)&data[0], size);
	}

	metaFile.close();

	fileName = convertFilename(fileName);
	metaFile.open(fileName, std::ios::binary | std::ios::out | std::ios::in);
	if (metaFile.is_open() == false)

	{

		new Error(Error::ErrorType::file, "file could not be re-opened");

		std::cout << "Error - meta file could not be re-opened \n";
		return;
	}
	metaFile.seekp(0, std::ios::beg);
	metaFile << Settings::version<<"\n";

	if (url.length() > 1)
	{
		metaFile << "2\n";
		metaFile << url << "\n";
	}
	else
	{
		metaFile << "0\n";
		metaFile << "this\n";
	}

	metaFile << position << "\n";
	metaFile << data.size() << "\n";


	metaFile << compressedSize << "\n";
	metaFile << sanity_string << "\n";
	metaFile.close();





}

 
Landscape* Cache::loadMetaData(std::string fileName)
{

	fileName = convertFilename(fileName);
	std::cout << " Load metadata " << fileName << "\n";


	metaFile.open(fileName, std::ios::in);

	


	std::string ver;
	metaFile >> ver;

 

	if (ver.length() > 10)
	{
 
			reportError("This .lcms file is not valid\nPlease reload from .raw or .mzml file.");
			return NULL;
 

	}
	bool supported = false;
	for (const std::string &allv : Globals::supported_versions)
		if (ver == allv)
			supported = true;

	std::cout << " version " << ver << "\n";
	if (!supported)
	{
		reportError("This .lcms file is no longer supported\nPlease reload from .raw or .mzml file.");
		return NULL;
	}

	std::string sourceType;
	metaFile >> sourceType;
	if (sourceType == "3")
	{
		std::vector<std::string> script;
		std::string line;
		std::getline(metaFile, line);

		while (!metaFile.eof())
		{
			std::getline(metaFile, line);
			rtrim(line);

			if (line.length() < 2)
				break;
			script.push_back(line);
 
		}

		Globals::script = script;
		metaFile.close();

		return NULL;
		//need to open 
	}

	std::string sourceURL;
	metaFile >> sourceURL;





	long long offset = -1;
	metaFile >> offset;
	std::cout << "offset " << offset  << "\n";
	long long metaSize;
	metaFile >> metaSize;
	long long metaCompressedSize;
	metaFile >> metaCompressedSize;
	std::cout << "metaCompressedSize " << metaCompressedSize << "\n";
	std::string sanity;
	metaFile >> sanity;

	metaFile.close();
	if (sanity != sanity_string)
	{
		new Error(Error::ErrorType::file, "The lcms file is not valid\n"+   fileName );
		return NULL;
	}
	if (offset < 1)
	{
		new Error(Error::ErrorType::file, "The lcms file is not valid\n" +  fileName );
		return NULL;
	}

	if (sourceType == "2")
	{
		LCHttp::loadUrl = sourceURL;
		loadFromURL = true;
		cacheCacheFile = false;
	}


	std::cout << " opening as binary" << ver << "\n";
	metaFile.open(fileName, std::ios::in | std::ios::binary | std::ios::ate);

	long long size = metaCompressedSize;


	std::vector<byte> buffer(size);
	metaFile.seekg(offset, std::ios::beg);

	metaFile.read((char*)&buffer[0], size);
	if (metaCompressedSize < metaSize)
	{
		std::cout << "decompressing" << metaSize << "\n";

		std::vector<byte> de_buffer(metaSize);
		int length = Zip::UncompressData(&buffer[0], metaCompressedSize, &de_buffer[0], metaSize);
		assert(length == metaSize);
		buffer = de_buffer;
	}

	std::cout << "prepare landscape" << metaSize << "\n";

	Landscape* l = new Landscape();

	l->setTexture(Render::Texture);
	std::cout << "deserialise landscape" << metaSize << "\n";

	l->deSerialiseData(buffer, Builder::viewLod);

	metaFile.close();
	

	std::cout << "mz Range " << l->worldMzRange.min << " - " << l->worldMzRange.max << "\n";
	std::cout << "lc Range " << l->worldLcRange.min << " - " << l->worldLcRange.max << "\n";

	return l;
}


#include "gl3w/gl3w.h"  

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
#include "MzmlLoader2.h"

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
#include "SystemSetup.h"
#include "gui.h"

int Builder::useThreads = 4;


int Builder::totalScans = 0;
volatile std::atomic<int> Builder::num_threads = 0;
std::mutex Builder::make_lock;

static const int pointsPerTile = 3000  ;
static const 	int pointsPerTileWeb = 3500;

//there are very few first-level tiles, they can afford to be bigger
//and it's the first view that people see, so don't make it too rough
static const int pointsPerTile0 = pointsPerTile *2 ;
static const int pointsPerTile1 = pointsPerTile * 3/2;

Builder::errorType Builder::error = none;

//absolute max number of threads to use
volatile int making[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };


Tile* Builder::makeTiles(MZData* data, int lod, int threadId)
{
	//	auto thread_id = std::this_thread::get_id();


	if (Globals::closing)
		return NULL;

	int lcSize = (int)data->getScans().size();
	int mzSize = data->getMaxSize();
	double ratio = (mzSize * 1.0) / lcSize;

	double size = pointsPerTile;
	if (Cache::makeMetaFile)
		size = pointsPerTileWeb;
	if (lod == 0)
		size = pointsPerTile0;

	int newY = int(sqrt(size / ratio) + .5);
 
	if (lod == 0)
		newY = sqrt(lcSize);

	if (newY < 8)
		newY = 8;
	int newX = int((size / newY) + .5);


	if (newX < 2)
		newX = 2;

 

	if (newY > lcSize)
		newY = lcSize;

	if (newX > mzSize)
		newX = mzSize;

	if ((lcSize * mzSize > size))
	{
	 
		 std::cout <<lod << " : " << size << " ~ " << newX * newY << "    : " << newX << " , " << newY << "   :  " << mzSize << " , " << lcSize << "\n";
		
	 

		
		MZData* smallData = data->reduce(newX, newY);
		DataSource source = { 0,0 };
		Tile* tile = new Tile(lod, source, System::primary);
		tile->setMZData(smallData);

		//  trying different splits - greater splits should mean fewer levels of detail needed
#if TOC_VERSION

		int splitX = 5;
		int splitY = 5;

		if (lod == 0)
		{
			splitX = 16;
			splitY = 4;

		}
		
		if (0)
		if (4*mzSize < lcSize )
		{
			splitX = 1;
			splitY = 4;
			 
		}
	 
		if (mzSize > lcSize * 6)
		{
			splitX = 6;
			splitY = 3;
			if (mzSize > 1000)
				splitX = 12;
 
		}

		 
		#else
		int splitX = 3;
		int splitY = 3;


		if (mzSize > lcSize * 6)
		{
			splitX = 6;
			splitY = 2;

		}
		if (lod == 0)
		{
			splitX = 12;
			splitY = 4;

		}

#endif
 

		auto children = data->split(splitX, splitY);
		//split should delete the scans, but not the MZData object itself
		delete data;

		for (auto child : children)
		{
			if (lod == 0)
				child->setRange(true);
			Tile* child_tile = makeTiles(child, lod + 1, threadId);

			if (child_tile != NULL)
				tile->addChild(child_tile);


		}


		if (1)
		{
			auto dataBuffer = tile->serialiseData(threadId);
			DataSource d = Cache::putData(dataBuffer);
			tile->setSource(d);
		}


		if (lod ==  viewLod)
		{

		//	std::cout << " added tile " << tile->id << "  " << tile->LOD << " \n";

			System::primary->addTile(tile);
			totalScans += tile->getMZData()->getScans().size();

		}
		else
		{
 

			tile->clearRAM();

		}

		return tile;

	}


	//	data->clear();
	//	return NULL;


		//here are highest level of detail
		// (These should be 'ragged' tiles)

	DataSource source = { 0,0 };
	data->setJagged(true);
	Tile* tile = new Tile(lod, source, System::primary);
	tile->setMZData(data);

	auto dataBuffer = tile->serialiseData(threadId);
	DataSource d = Cache::putData(dataBuffer);
	tile->setSource(d);

	//this should only happen on very small datasets
	if (lod == viewLod)
	{
		System::primary->addTile(tile);

	}
	else
	{
		tile->clearRAM();

	}

	return tile;
}


std::mutex testLock;

Tile* Builder::makeTilesBase(MZData* data, int threadId)
{


	//should probably already be set
	making[threadId] = 1;
	bool locked = false;
	if (num_threads >= useThreads)
	{
		testLock.lock();
		locked = true;
	}
	//	make_lock.lock();

	Tile* result = makeTiles(data, 0, threadId);

	//make tiles should delete data

//	make_lock.unlock();

	//clear the slot
	making[threadId] = 0;
	if (locked)
	{
		testLock.unlock();
	}

	num_threads--;
	return result;
}


void Builder::makeLandscapeFromCache(std::string filename)
{
 

	Landscape* l = Cache::loadMetaData(filename);
	if (l != NULL)
	{
		System::primary = l;
		Cache::setupCache(filename, false);

		setCallbacks(System::primary);
		Globals::statusText = "";

	}

}
void Builder::makeLandscapeFromData(std::vector<byte> data)
{

}


extern int sax_error_type;



void Builder::makeLandscape(std::string filename)
{

	if (System::primary != NULL)
		delete System::primary;


	System::primary = new Landscape();

	System::setPrimaryTexture();

	Cache::setupCache(filename, true);

	//	std::cout << " mesh setup \n" << std::flush;
	std::string loadFile = tolower(filename);


	// std::string filename = "files/small.mzml"; // small.mzml  test2.mzml
	MZLoader* loader;

	//check if a custom reader exists
	std::string exe = gui::getFileReader(loadFile);


	if (exe.length() > 0)
	{
		loader = new RawLoader(filename, exe);

	}
	else
	{


		if (endsWith(loadFile, ".raw"))
		{
#ifdef USE_RAW_READER
			loader = new RawLoader(filename, RawLoader::rawReaderPath);
#else
			new Error(Error::ErrorType::file, "Raw files can only be loaded directly with Windows drivers installed.\nPlease convert to mzml.");
			delete System::primary;
			System::primary = NULL;
			Globals::statusText = "";
			return;
#endif

		}
		else
		{
			if (Settings::experimentalMzml)
				loader = new MzmlLoader(filename);
			else
				loader = new MzmlLoader2(filename);
		}
		//loader = new MzmlLoader(filename);
	}


	//	std::cout << "prepared mzml file \n";
	System::primary->setDrawCallback(NULL);
	System::primary->setMakeMeshCallback(NULL);

	//	primary->setMakeMeshCallback(makeMeshStandard);
	//	primary->setDrawCallback(drawTile);


	//	primary->addTile(aTile);

	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
	error = none;

	while (true)
	{
		if (Globals::closing)
		{
			std::cout << "abort load1 \n" << std::flush;
			return;
		}

		MZData* newData = loader->loadData();


		if (newData == NULL)
			break;

		if (newData->size() == 0)
			continue;

#if 1

		testLock.lock();
		testLock.unlock();


		//find a slot that is not in use
		for (int i = 0; i < useThreads; i++)
		{

			if (making[i] == 0)
			{
				num_threads++;
				int nt = i;
				std::thread t1(makeTilesBase, newData, nt);
				making[nt] = 1;

				//makeTilesBase(newData, nt);
				t1.detach();
				break;
			}
		}



#endif


	}

	while (num_threads > 0)
	{
		if (Globals::closing)
		{
			std::cout << "abort load2 \n" << std::flush;
			return;
		}
	};



	make_lock.lock();
	make_lock.unlock();
	System::primary->addPendingTiles();



	if (error != none)
	{
#if TOC_VERSION
		std::string error_string = "The data file was not sequential\n";
#else
		std::string error_string = "The data file was not sequential\nFor mzml, please try converting it with ProteoWizard.";
#endif
		if (error == exception)
			error_string = "The data file could not be read.";

		new Error(Error::ErrorType::file, error_string);
		delete loader;
		delete System::primary;
		System::primary = NULL;
		Globals::statusText = "";

		Cache::closeCache();
		std::cout << "abort load3 \n" << std::flush;
		return;
	}




	if (System::primary->getTiles().size() == 0)
	{
		std::string error = "No data was found in the file.";
		if (Settings::noiseRemoval)
			error = "No data was found in the file (after small values removed).";

		if (endsWith(loadFile, ".mzml"))
		{

			error = "No LC-MS data was found in the .mzml file.";

			if (Settings::experimentalMzml == false)
			{
				error = "No LC-MS data was found in the .mzml file. ";
				if (sax_error_type == 1)
					error = "No LC-MS data was found in the .mzml file. For non-indexed files, \nplease try enabling the experimental mzml reader in the advanced settings\nor convert the file to indexed.";
			}
		}
		if (endsWith(loadFile, ".raw"))
			error = "No LC-MS data was found in the .raw file.";
		

		new Error(Error::ErrorType::file, error);
		delete loader;
		delete System::primary;
		System::primary = NULL;
		Globals::statusText = "";

		Cache::closeCache();

		std::cout << "abort load4 \n" << std::flush;
		return;
	}


	std::cout << "loaded mzml file " << System::primary->getTiles().size() << " \n";
	std::chrono::system_clock::time_point end = std::chrono::system_clock::now();

	std::cout << " build took " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " \n";


	if (Cache::makeMetaFile)
	{ // make an internet version with just metadata
		std::string metaFilename = filename + "_web";
		auto flags = std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc;

		std::cout << " creating " << metaFilename + Cache::cacheFileEnding << "\n";
		std::fstream metaFile;

		metaFilename = convertFilename(metaFilename);

		metaFile.open(metaFilename + Cache::cacheFileEnding, flags);

		Cache::insertHeaderSpace(metaFile);
		metaFile.close();

		auto urlFilename = filename;
		const size_t last_slash_idx = urlFilename.find_last_of("\\/");
		if (std::string::npos != last_slash_idx)
		{
			urlFilename.erase(0, last_slash_idx + 1);
		}


		Cache::saveMetaData(System::primary, metaFilename, "http://pgb.liv.ac.uk/~tony/lcms/getdata.php?file=" + urlFilename + Cache::cacheFileEnding);
	}


	//add the metadata to the file that was just created
	Cache::cacheFile.close();

	Cache::saveMetaData(System::primary, filename, "");
	auto flags = std::ios::in | std::ios::binary;

	filename = convertFilename(filename);

	Cache::cacheFile.open(filename + Cache::cacheFileEnding, flags);
	setCallbacks(System::primary);


	MZData::cleanUp();

	delete loader;


	std::cout << "end load \n" << std::flush;
	// Globals::statusText = "Loading complete ";
	return;


}


void Builder::setCallbacks(Landscape* l)
{
	std::cout << " Callbacks set \n";
	l->setMakeMeshCallback(Cache::makeMeshStandard);

	l->setLoadCallback(Cache::loadDataFromCache);

	l->setDrawCallback(Render::drawTile);

	l->readyToDraw();

	std::thread t1(Cache::processLoadQueue);
	t1.detach();
	std::thread t2(Cache::processmakeQueue);
	t2.detach();

}
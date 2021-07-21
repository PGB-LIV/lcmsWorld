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

static const int pointsPerTile = 4500;
static const 	int pointsPerTileWeb = 3500;
static const int pointsPerTile0 = pointsPerTile *3;

Builder::errorType Builder::error = none;

//absolute max number of threads to use
volatile int making[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };


Tile* Builder::makeTiles(MZData* data, int lod, int threadId)
{
	//	auto thread_id = std::this_thread::get_id();

	


	int lcSize = (int)data->getScans().size();
	int mzSize = data->getMaxSize();
	double ratio = (mzSize*1.0) / lcSize;

	double size = pointsPerTile;
	if (Cache::makeMetaFile)
		size = pointsPerTileWeb;

	if (lod == 0)
	{
		size = pointsPerTile0;

	}


	int newY = int(sqrt(size / ratio) + .5);
	int newX = int((size / newY) + .5);

	if (newY < 8)
		newY = 8;
	if (newX < 8)
		newX = 8;

	if ((lcSize * mzSize > pointsPerTile))
	{

		MZData* smallData = data->reduce(newX, newY);
		DataSource source = { 0,0 };
		Tile* tile = new Tile(lod, source, System::primary);
		tile->setMZData(smallData);

//  trying different splits - greater splits should mean fewer levels of detail needed

		int splitX = 2;
		int splitY = 2;
		if (lod < 2 )
		{
			splitX = 4;
			splitY = 1;

		}

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


		if (lod == viewLod)
		{

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


extern int sax_error_type ;



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
	MZLoader *loader;

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
				t1.detach();
				break;
			}
		}



#endif


	}

	while (num_threads > 0);



	make_lock.lock();
	make_lock.unlock();
	System::primary->addPendingTiles();



	if (error != none)
	{
		std::string error_string = "The mzml file was not sequential\nPlease try converting it with ProteoWizard.";
		if (error==exception)
			error_string = "The mzml file could not be read.";

		new Error(Error::ErrorType::file, error_string);
		delete loader;
		delete System::primary;
		System::primary = NULL;
		Globals::statusText = "";

		Cache::closeCache();
		return;
	}




	if (System::primary->getTiles().size() == 0)
	{
		std::string error = "No LC-MS data was found in the .mzml file.";
		
		if (Settings::experimentalMzml == false)
		{
			error = "No LC-MS data was found in the .mzml file. ";
			if (sax_error_type==1)
				error = "No LC-MS data was found in the .mzml file. For non-indexed files, \nplease try enabling the experimental mzml reader in the advanced settings\nor convert the file to indexed.";
		}

		if (endsWith(loadFile, ".raw"))
			error = "No LC-MS data was found in the .raw file.";

		new Error(Error::ErrorType::file, error);
		delete loader;
		delete System::primary;
		System::primary = NULL;
		Globals::statusText = "";

		Cache::closeCache( );


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

 
	std::cout << std::flush;
	// Globals::statusText = "Loading complete ";
	return;


}


void Builder::setCallbacks(Landscape *l)
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
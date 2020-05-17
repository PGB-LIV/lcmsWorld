#pragma once
#include <atomic>
#include <mutex>
class MZData;

class Builder
{

 

 	static int totalScans ;
	volatile static std::atomic<int> num_threads;
	static std::mutex make_lock;
	static void setCallbacks(Landscape *l);

public:
	enum errorType { none, nonSeqMzml };
	
	static int useThreads;
	static errorType error;
 	static const int viewLod = 0;
	static Tile* makeTiles(MZData* data, int lod, int threadId);
	static Tile* makeTilesBase(MZData* data, int threadId);
	static void makeLandscapeFromCache(std::string filename);
	static void makeLandscape(std::string filename);
	static void makeLandscapeFromData(std::vector<byte> data);

};
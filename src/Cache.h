#pragma once
#include <queue>
#include <condition_variable>
class Cache
{

	static std::streamoff cachePosition ;

	static std::streamoff  cacheFileSize;
	static bool loadFromURL;
	static bool compressData;
	static std::mutex meshLock;
	static std::mutex cacheLock;

	static std::vector<byte> dataCache;

	static std::mutex loadmtx;
	static std::condition_variable loadcv;
	static std::vector<Tile*> loadQueue;
	static std::mutex loadQueueLock;
	static int make_tiles;
	static int made_tiles;


	static std::mutex makemtx;
	static std::condition_variable makecv;
	static std::queue<Tile*> makeQueue;
	static std::mutex makeQueueLock;
	static bool cancelling;
	static std::fstream metaFile;

public:
	static int queueSize;

	static bool cacheCacheFile;
	static bool processQueue;

	static std::string cacheFileEnding;
	static std::fstream cacheFile;
	static bool makeMetaFile;
	static bool loadQueueEnded;
	static bool makeQueueEnded;


	static void insertHeaderSpace(std::fstream &outFile);
	static void setupCache(std::string inputFileName, bool newFile);
	static inline std::vector<byte> getDataFromFile(DataSource d);
	static void loadCache();
	static std::vector<byte> getData(DataSource d);
	static DataSource putData(const std::vector<byte> & data);
	static void processLoadQueue();
	static void processmakeQueueFG();
	static void loadDataFromCache(Tile* tile);
	static void closeCache();
	static void processmakeQueue();
	static void makeMeshStandard(Tile* tile);
	static void saveMetaData(Landscape *l, std::string inputFileName, std::string url);
	static Landscape*  loadMetaData(std::string fileName);

};

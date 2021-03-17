
#include "SystemSetup.h"
#include "MemsizeUtil.h"
#include <iostream>
#include "Utils.h"
#include "Cache.h"
#include "Render.h"
#include "Builder.h"

const static int maxThreads = 6;
 Landscape *System::primary = NULL;
 double System::frameTime = 1;
  int64 System::loopCount = 0;
    double System::systemMemory = 1.0;


 void System::setPrimaryTexture()
 {
	 primary->setTexture(Render::Texture[0]);

 }
void System::setup()
{

	auto memsize = MemsizeUtil::getMemorySize();
	std::cout << "memory = " << memsize / 1024.0 / 1024  / 1024 << " gb\n";
	systemMemory = memsize / 1024.0 / 1024 / 1024;
	
	if (memsize > (1117LL * 1024 * 1024 * 1024))
	{
		std::cout << "cache binary data enabled \n";
		Cache::cacheCacheFile = true;
	}
	else
	{
		std::cout << " not caching binary data \n";
	}
	int cores = Utils::getNumberOfCores();
	std::cout <<  cores << " cores reported \n";

	int useThreads = std::max(1,cores/2);
	if (Cache::makeMetaFile)
		useThreads = cores;

	if (memsize <= 1024 * 1024 * 1024)
		useThreads = std::min(useThreads, 2);
	if (memsize <= 2LL * 1024 * 1024 * 1024)
		useThreads = std::min(useThreads, 3);
	if (memsize <= 6LL * 1024 * 1024 * 1024)
		useThreads = std::min(useThreads, 5);
	if (memsize <= 15LL * 1024 * 1024 * 1024)
		useThreads = std::min(useThreads, 11);

	useThreads = std::min(useThreads, maxThreads);

 
	Builder::useThreads = useThreads;
	std::cout << "using " << useThreads << " threads for loading \n";

}

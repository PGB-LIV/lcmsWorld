#pragma once
#include "Landscape.h"
class System
{

public:
	static void setup();
	static Landscape *primary ;
	static void setPrimaryTexture();
	static double frameTime;
 	static int64 loopCount;
	static double systemMemory;

};
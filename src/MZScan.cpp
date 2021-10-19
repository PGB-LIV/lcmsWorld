#include "MZScan.h"
#include <iterator>
#include <algorithm>

int numMZScans = 0;
int delMZScans = 0;

 
MZScan::MZScan(MZScan* ptr)
{
	
	parent = NULL;
	intensity = ptr->intensity;

	mz = ptr->mz;

	type = ptr->type;
	lcTime = ptr->lcTime;
	size = ptr->size;
	id = numMZScans++;

}
MZScan::MZScan(  std::vector<signalFloat> intensityVals, lcFloat time )
{
	parent = NULL;
	intensity = intensityVals;
	
	type = MZDataType::square;
	lcTime = time;

	id = numMZScans++;

}
MZScan::MZScan(  std::vector<mzFloat> mzVals, std::vector<signalFloat> intensityVals, lcFloat time)
{
	parent = NULL;
	intensity = intensityVals;
	mz = mzVals;


	type = MZDataType::jagged;
	lcTime = time;
	id = numMZScans++;

#ifdef FINAL
	// no need to check for these errors in build
#else

	if (true)
	{
		double lastmz = -1e22;
		for (auto mz : mzVals)
		{
			if (mz < lastmz)
			{
				std::cout << "MZ ordering error \n";
			}
			lastmz = mz;

		}
	
	}
#endif
}




MZScan::MZScan(std::vector<signalFloat> intensityVals, lcFloat time, MZData* p)
{
	parent = p;
	intensity = intensityVals;

	type = MZDataType::square;
	lcTime = time;
	id = numMZScans++;

}
MZScan::MZScan(std::vector<mzFloat> mzVals, std::vector<signalFloat> intensityVals, lcFloat time, MZData* p)
{
	parent = p;
	intensity = intensityVals;
	mz = mzVals;
	type = MZDataType::jagged;
	lcTime = time;
	id = numMZScans++;



	if (true)
	{
		double lastmz = -1e22;
		for (auto mz : mzVals)
		{
			if (mz < lastmz)
			{
				std::cout << "MZ ordering error \n";
			}
			lastmz = mz;

		}

	}


}

MZScan::~MZScan()
{
	delMZScans++;
}

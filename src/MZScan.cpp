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
	numMZScans++;

}
MZScan::MZScan(  std::vector<signalFloat> intensityVals, lcFloat time )
{
	parent = NULL;
	intensity = intensityVals;
	
	type = MZDataType::square;
	lcTime = time;

	numMZScans++;

}
MZScan::MZScan(  std::vector<mzFloat> mzVals, std::vector<signalFloat> intensityVals, lcFloat time)
{
	parent = NULL;
	intensity = intensityVals;
	mz = mzVals;

	type = MZDataType::jagged;
	lcTime = time;
	numMZScans++;
}




MZScan::MZScan(std::vector<signalFloat> intensityVals, lcFloat time, MZData* p)
{
	parent = p;
	intensity = intensityVals;

	type = MZDataType::square;
	lcTime = time;
	numMZScans++;

}
MZScan::MZScan(std::vector<mzFloat> mzVals, std::vector<signalFloat> intensityVals, lcFloat time, MZData* p)
{
	parent = p;
	intensity = intensityVals;
	mz = mzVals;
	type = MZDataType::jagged;
	lcTime = time;
	numMZScans++;
}

MZScan::~MZScan()
{
	delMZScans++;
}

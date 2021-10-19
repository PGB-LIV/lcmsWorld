#pragma once

#include "MZData.h"
class MZLoader
{
public:
	MZLoader();
	virtual ~MZLoader()=0;
	virtual MZData* loadData() = 0; 
	size_t getFileSize();
	double getProgress();
protected:
	const int linesPerChunk = 990*3;


	SignalMz insertZeros(const std::vector<mzFloat> &mz, const std::vector<signalFloat> &sig);
	void addScan();
	MZData  *result = NULL;
	lcFloat lcTime = 0;

	std::vector<mzFloat> mzData;
	std::vector<signalFloat> intensityData;
	MZScan *last_line = NULL;
	int lcount = 0;

};


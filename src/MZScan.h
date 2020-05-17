#pragma once

#include <vector>
#include "Structs.h"
 

class MZData;

// represents a single scan of MZdata
// may include both mz and intensity
// but for square data, mz is not needed

class MZScan
{
public:
 

	MZScan(std::vector<signalFloat> intensityVals, lcFloat time);

	MZScan(std::vector<mzFloat  > mz, std::vector<signalFloat> intensityVals, lcFloat time);

	MZScan(std::vector<signalFloat> intensityVals, lcFloat t, MZData* pt);

	MZScan(std::vector<mzFloat  > mz, std::vector<signalFloat> intensityVals, lcFloat t, MZData* pt);

	~MZScan();
	inline  lcFloat getLcTime() { return lcTime; }
	const inline  std::vector<mzFloat> &getMz() { return mz; }
	const inline  std::vector<signalFloat> &getIntensity() { return intensity; }
	inline MZDataType getType() { return type; }
	inline int getSize() {return (int) intensity.size();}
	MZData* parent;
	MZScan(MZScan* ptr);
	void blank() {
		for (unsigned int i = 0; i < intensity.size(); i++)
			intensity[i] = 0;
//		std::fill<signalFloat>(intensity.begin(), intensity.end(), 0);
		lcTime += 0.1f;
	}
private:	
	lcFloat lcTime = 0;
	int size = 0;
	std::vector<mzFloat> mz;
	std::vector<signalFloat> intensity;
	 
	MZDataType type = MZDataType::jagged;

};


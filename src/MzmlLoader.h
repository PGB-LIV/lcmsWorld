#pragma once
#include "MZLoader.h"
#include "MZLoaderHandle.h"
class MzmlLoader :
	public MZLoader
{
public:
	MzmlLoader(MZLoaderHandle filehandle);
	~MzmlLoader();
		MZData* loadDataPartial();
	MZData* loadData();
	long long getFileSize();
	double getProgress();
	

private:

	MZLoaderHandle fileHandle;
	std::vector<double> readBinary(std::string_view const & line);
	std::vector<float> readBinaryFloat(const std::string_view  &line);
	std::vector<mzFloat> readBinaryMz(const std::string_view  &line);
	std::vector<signalFloat> readBinarySignal(const std::string_view  &line);
	SignalMz insertZeros(const std::vector<mzFloat> &mz, const std::vector<signalFloat> &sig);
	bool  readMz(const std::string_view  &line);
	bool readIntensity(const std::string_view &line);
	void processLine(const std::string_view & line);
 
	int function = 0;
	long long index = 0;
	int getData = 0;
	int bigData64 = 0;
	int compressedData = 0;
	int level = 1;
	bool useSeconds = false;



	int count = 0;

	std::vector<unsigned char> zipBuffer;
	int zipBufferSize = 65536 * 64;


};


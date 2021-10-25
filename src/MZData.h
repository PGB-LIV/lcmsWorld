#pragma once
#include <iostream>  
#include <set>
#include "Structs.h"
#include "MZScan.h"
#include "MZLoaderHandle.h"
#include "Mesh.h"

class MZData
{
public:
	MZData();

	// MZData(const std::vector<byte> &buffer);
	~MZData();
	friend class Tile;
	friend class Landscape;
 	void append(MZScan* scan);
	friend std::ostream& operator<<(std::ostream& os, const MZData& dt);
	double setRange(bool setIntensity = false);


	MZData* reduce(int xsize, int ysize);
	void merge(MZData* o);
	
	inline const std::vector<MZScan*> &getScans(){ return scans; }
	std::vector<MZData*> split(int xsize, int ysize);
	void setJagged(bool jag) { type = jag ? MZDataType::jagged : MZDataType::square ; }
	static int reduceType;
	
	const std::vector<byte> serialise(int thread);
	const int serialise(byte * buffer);
	const std::vector<byte> serialise(int thread, bool compress);

	inline int size() { return (int) scans.size(); }
	inline int getMaxSize() { return maxSize; }
	void clear();
	void deSerialise(const std::vector<byte> &buffer);
	static inline glm::vec3  transform(glm::vec3  input);
	std::string getInfoString();

	struct MZDataInfo info;


#if 0
	static void setScale(Tile* tile);

	static double xScale;
	static double yScale;
	static double zScale;
	static worldView viewData;
	Mesh* getJaggedMesh(Tile* tile);
	bool insertData(Tile* tile, std::vector<glm::vec3> &vertex_vec, std::vector<glm::vec2> &uv_vec, mzFloat x1, mzFloat x2, lcFloat y1, lcFloat y2, signalFloat z1, signalFloat z2, signalFloat z3, signalFloat z4);
	static bool scalingValuesSet;

#endif
	std::vector<DataPoint> findClosePointsinLine(DataPoint s, MZScan* line, MZScan* base);

	std::vector<DataPoint>  findClosePoints(DataPoint s);
		std::vector<DataPoint>  findClosest(DataPoint s);
	static void cleanUp();

	int id = 0;
private:
 
	int state = 0;

	static int next_id;

	int times_cleared = 0;
 
	//the binary data linked to this particular piece of data
	// (i.e., an identifier used by the loader?)
	// MZLoaderHandle fileInfo;

	//if the data is 'square'- i.e., every scan gas the same mz values
	// then only the first scan will contain them
	void copyRange(MZData* source);
 

	std::vector<MZScan*> scans;
 
	MZDataType type = MZDataType::jagged;
//	const static double scaleFactor;

	double signalSum = 0;


//	const static double peakUV;
 
	int maxSize = 0;



	
	
	// Store a link to a Mesh object which is attached to / created from this MZData
	Mesh* mesh = NULL;

	std::vector<MZData*> old_split(int xsize, int ysize);
	std::vector<MZData*> new_split(int xsize, int ysize);
	
};



#pragma once

#include "Structs.h"
#include "Mesh.h"
#include "GLMesh.h"
#include "MZData.h"
#include <mutex>
#include <deque>
#include "Fader.h"
 
#include "Landscape.h"

class Tile
{
public:
	Tile(int lod, DataSource src, Landscape* o);
	Tile(Landscape* o)
	{
		owner = o;
		random = std::rand();

	}
	
	~Tile();

	MZDataType type;

	GLMesh* getGLMesh() { return glMesh; }
	GLMesh* getwGLMesh() { return wglMesh; }
	Mesh* getMesh() { return mesh; }
	//
	GLMesh* setGLMesh(bool recreate = false);
	
 
 
	void setMesh(Mesh* m);
	void updateWorldRange()
	{
		if (mzdata!=NULL)
		{
			owner->scalingValuesSet = false;
			mzdata->setRange();
			updateWorldRange(mzdata->info);
		}
	}
	void setSource(DataSource d)
	{
		source = d;
	}



	const std::vector<byte> serialiseData(int thread_id);

	const std::vector<byte> serialise();
	int deserialise(byte* buffer);
	int  serialise(byte *buffer);

	void deSerialiseData(const std::vector<byte> &buf);

	DataSource getSource() { return source; }

	void setMZData(MZData* n);


	MZData* getMZData() { return mzdata; }

	DrawStatus drawStatus = DrawStatus::noData;
	void setScreenSize(glm::mat4 matrix, glm::vec2 view);
	
 
	double getScreenSize() { return screenArea; }
	void clearGLMesh(bool rebuild);
	void clearMesh(bool rebuild);
	void clearGLMesh() { clearGLMesh(false); }
	void clearMesh() {
		clearMesh(false);
	}
	void clearMZData();
	
	void clearRAM(){
		clearGLMesh();
		clearMesh();
		clearMZData();
		drawStatus = DrawStatus::noData;
	}


 

//	DrawStatus getChildrenStatus();
	const std::vector< Tile*> &getChildren() { return children; };

	//we are originally going to restore the childIds
   // only when we are finished, can we restore the pointers
	static std::vector<Tile*> tileBuffer;

	Range<mzFloat> getWorldMzRange() {
		return owner->worldMzRange;
	}
	Range<lcFloat> getWorldLcRange() {
		return owner->worldLcRange;
	}
	Range<signalFloat> getWorldSignalRange() {
		return owner->worldSignalRange;
	}
	void updateWorldRange(MZDataInfo info);
	void addChild(Tile* tile) { children.push_back(tile); tile->parent = this; }
	// a tile to be loaded into memory
	unsigned int id = 0;
	int LOD = 0;
	Fader fader;

	static void setChildren();
	Range<mzFloat> mzRange;
	Range<lcFloat> lcRange;
	inline double getCameraDistance() { return cameraDistance; }

	// notified that the tile is ready to be drawn, may not actually be visible
	inline void beingDrawn()
	{
		lastDrawn = Globals::getCurrentTime();
		auto p = parent;
		while (p != NULL) {
//			if (p->id < 2)
//				std::cout << " set p " << p->id << "\n";
			p->beingDrawn();
			p = p->parent;
		}

	}
	inline bool isDrawnOnScreen()
	{
		if (Globals::getCurrentTime().time - lastVisible.time < visTime)
			if (Globals::getCurrentTime().time - lastDrawn.time < visTime)
				return true;
		return false;
	}
	inline bool isBeingDrawn()
	{
		
			if (Globals::getCurrentTime().time - lastDrawn.time < visTime)
				return true;
		return false;
	}
 
	bool unLoad();

	// use this to see if it is actually going to be visible
	inline bool isOnScreen()
	{
	 
		return (Globals::getCurrentTime().time - lastVisible.time < visTime);
	}

 
	void setScreenSize()
	{
 
		setScreenSize(owner->transformMatrix, owner->viewport);
	}
	inline bool hasData()
	{
		if (drawStatus > DrawStatus::loadingData)
		{
			return mzdata != NULL;
		}
		return false;
	}


	std::vector<DataPointInfo> findClosest(DataPoint s)
	{
	// 	DataPointInfo d = { s.mz,s.lc,s.signal,-1 };
		std::vector<DataPointInfo> d;

		if (mzdata == NULL)
			return d;
		if (isBeingDrawn() == false)
			return d;
		if (!hasMzData)
			return d;

 		auto res = mzdata->findClosest(s);
		d.reserve(res.size());
		for (auto p : res)
		{
			d.push_back(DataPointInfo{ p.mz,p.lc,p.signal,LOD });
		}
 		
		return (d);
	}

	std::vector<DataPointInfo> findClosePoints(DataPoint s)
	{
		// 	DataPointInfo d = { s.mz,s.lc,s.signal,-1 };
		std::vector<DataPointInfo> d;

		if (mzdata == NULL)
			return d;
		if (isBeingDrawn() == false)
			return d;
		auto res = mzdata->findClosePoints(s);
		d.reserve(res.size());
		for (auto p : res)
		{
			d.push_back(DataPointInfo{ p.mz,p.lc,p.signal,LOD });
		}

		return (d);
	}


	double cachePriority;

	void storePriority()
	{
		cachePriority = lastDrawn.time + id/1000; // +screenArea; //  + screenArea + LOD / 10 + id / 100;
	}

 
	static bool compareTilePtrReverse(Tile* a, Tile* b) {

 

		return (a->cachePriority > b->cachePriority);

		
	
	}
	
	float cacheLocation;
	void storeLocation()
	{

		cacheLocation = screenLocation;

 

	}

	//beware that this may change - may need to cache values
	static bool compareTilePtr(Tile* a, Tile* b) {
//		if ((Globals::getCurrentTime().time - a->lastVisible.time < visTime) != (Globals::getCurrentTime().time - b->lastVisible.time < visTime))
//			return ((Globals::getCurrentTime().time - a->lastVisible.time < visTime) < (Globals::getCurrentTime().time - b->lastVisible.time < visTime));
		//		return a->screenArea < b->screenArea;

//		if (a->LOD != b->LOD)
//		return (a->LOD > b->LOD);

		if (a->cacheLocation * (a->LOD + 1) != b->cacheLocation * (b->LOD + 1))
		return (a->cacheLocation *(a->LOD+1) > b->cacheLocation * (b->LOD+1));

		return (a->id < b->id);
		//	return screenArea * LOD > rhs.screenArea*rhs.LOD;
		///
	}

	Landscape* owner = NULL;
	Tile * parent = NULL;
	TimeStamp childTime = { 0 };
	TimeStamp lastLoaded = { 0 };
	TimeStamp lastVisible = { 0 };

	// a tile may be visible, but too large/small to be drawn itself - this is when it was actually drawn
	TimeStamp lastDrawn = { 0 };
	void setScreenSizeMaxSibling(std::vector<Tile*> sibs);

private:
	signalFloat maxSignal;
	float screenLocation = 20;
	//when it was last on or near the screen somewhere - whether drawn or not

	
	bool hasMzData = false;

	float screenArea = -1;
	float lastScreenArea = -2;
	int random;

	int numDataPoints = 0;

	static float visTime ;

	float cameraDistance = -2;
	inline glm::vec4  transform(glm::vec3  input);

	//where to load data from
	DataSource source;
	
	
	// notification that it is visible
	inline void onScreenNow()
	{
 

		lastVisible = Globals::getCurrentTime();
		

		//temporary - pass it back up the tree so that parents are not unloaded
		//this should not be apermanent chage
		
		  return;
		auto p = parent;

		while (p != NULL) {
			if (p->id < 2)
				
			std::cout << " set p " << p->id << "\n";
			p->onScreenNow();
			p = p->parent;
		}
	}

	bool dataLoaded = false;
 
	
	MZData* mzdata = NULL;
	std::vector<Tile*> children;
	//may not be needed any more
//	std::vector<Tile*> new_children;
	

	
	
 	Mesh* mesh = NULL;
	GLMesh* glMesh = NULL;
	GLMesh* wglMesh = NULL;
	std::vector<int> childIds;

 


};


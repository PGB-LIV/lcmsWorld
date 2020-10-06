#pragma once

#define NOMINMAX

#include "gl3w/gl3w.h"  
#include "GLTextureID.h"
#include "glm/glm.hpp"
#include "Structs.h"
#include "Globals.h"
#include "GLMesh.h"
#include "Utils.h"
#include <mutex>
#include <deque>
#include <set>
#include <map>
#include <chrono>
#include <thread>
class Tile;
class Camera;
#define zOffset .001f
#define cacheHeaderSize 4096
bool compareAlpha(Annotation* a, Annotation* b);

class Landscape
{
	friend Tile;

public:
	Landscape();
	~Landscape();
	void setTexture(GLuint texture) {
		peakTexture = texture;
	}

	long long numDataPoints = 0;
	void setInfo();
	void addTile(Tile* newTile);
	void drawTiles();
	void setDrawCallback(void(*callback)(Tile* mesh, bool isFaded));
	void setLoadCallback(void(*callback)(Tile* mesh));
	void setMakeMeshCallback(void(*callback)(Tile* mesh));
	std::vector<Tile*> &getTiles() { return tiles; }
	void drawTiles(const std::vector<Tile*> tiles);
	void prepareTiles(const std::vector<Tile*> tiles);
	void updateLandscape(glm::mat4 matrix);
	void reMake();
	void updateViewport(int x, int y);
	glm::vec2 getViewport() { return viewport; }

	std::vector<Label> &getLabels(unsigned int index) {
		while (index >= labels.size())
		{
			std::vector<Label> a;
			labels.push_back(a);
		}
		return labels[index]; 
	};

	std::vector<Label> &getLabels() { return getLabels(0); };
	void addLabel(unsigned int index, Label label) {
		while (index >= labels.size())
		{
			std::vector<Label> a;
			labels.push_back(a);
		}
		labels[index].push_back(label); }
	void addLabel(Label label) { addLabel(0,label); }
	void clearLabels(int index) { labels[index].clear(); }
	void clearLabels() { for (unsigned int i=0; i < labels.size(); i++)
		clearLabels(i); }
	std::vector<byte> serialiseData();
	void deSerialiseData(std::vector<byte> buf, int viewLod);

 	Range<mzFloat> worldMzRange = { 3e37f,0 };
	Range<lcFloat> worldLcRange = { 3e37f,0 };
	Range<signalFloat> worldSignalRange = { 3e37f,0 };;

	void updateWorldRange(MZDataInfo info);
	std::deque<Tile*> tileList;
	std::set<Tile*> rebuildQueue;


	void registerTile(Tile* t);
	void reBuild();

	// this could use id and lookup table rather than map
	// value is not used for now anyway; will implement with queue
 
 

 

	void dataLoaded(Tile* tile);

	void tileDrawn(Tile* tile);
	void addAnnotation(Annotation a);
		void drawAnnotations();
	void drawCubes();
	void readyToDraw(){ readyToDrawFlag = true; }
	GLMesh* makeBaseMesh(GLuint textureId);
	GLMesh* makeBaseQuads(GLuint textureId);
	DataPoint findDataPoint(mzFloat mz, lcFloat lc, signalFloat sig);
	void addMarker(float tx, float ty, float tz, int i, std::string text, float width, float height, float cubeSize = 0);
	void add3dMarker(float tx, float ty, float tz, int i, std::string text, float width, float height, float cubeSize);
	std::vector<std::string> getInfo() { 
		std::vector<std::string> infoCopy(info);
		info.clear();

		return infoCopy; };
	void addInfo(std::string infoString) {
		info.push_back(infoString);
	}
	MZDataInfo filter;
	MZDataInfo lastFilter;
	std::vector<float>  getFilterModelSpace();
	Annotation getClosestAnnotation(DataPoint cursor);

	std::vector<Annotation*> &getAnnotationsByName() { return annotationsByName; };
	std::vector<Annotation> &getAnnotations();
	
	void setVisible(int num)
	{
		getAnnotations();
		num = std::min((int) annotations.size(), num);
		if (num == 0)
		{
			for (auto &a : annotations)
			{
				setVisible(&a, false);
			}

		}

		for (int i = 0; i < num; i++)
		{
			setVisible(annotationsBestFirst[i], true, Settings::mergeIdents);
		}

	}

	void setVisible(Annotation* a, bool isVisible, bool range)
	{
		if (!range)
		{
			setVisible(a, isVisible);
			return;
		}
		auto lower = std::lower_bound(annotationsByName.begin(), annotationsByName.end(), a, compareAlpha);
		std::string name = a->text;
		while (!strcasecmp((*lower)->text, name))
		{
			setVisible((*lower), isVisible);

			lower++;


			if (lower >= annotationsByName.end())
				break;
		}
	}

 

	void setVisible(std::string name, bool isVisible, bool range)
	{
	 
		Annotation* temp = new Annotation;
		temp->text = name;
		temp->score = 0;

		auto lower = std::lower_bound(annotationsByName.begin(), annotationsByName.end(), temp, compareAlpha);
 
		delete temp;
		while (!strcasecmp((*lower)->text, name))
		{
			setVisible((*lower), isVisible);

			lower++;


			if (lower >= annotationsByName.end())
				break;
		}
	}
	void scaleAnnotations(double scale_factor)
	{
		for (auto &a : annotations)
		{
 
			a.lc *= (lcFloat) scale_factor;
 
		}

	}
	void setVisible(Annotation* a, bool vis)
	{
		if (vis)
			visibleAnnotations.insert(a);
		else
			visibleAnnotations.erase(a);
		a->isVisible = vis;
	}
	std::set<Annotation*> visibleAnnotations;
	bool hasAnnotations() { return annotationsLoaded; }
	void setAnnotationsLoaded(double scale_factor) { scaleAnnotations(scale_factor); annotationsLoaded = true; setMap(); }
	Camera* getCamera() { return camera; }
	 void updateCamera(float time );
	glm::mat4 getCameraMatrix();
	void addPendingTiles();
	Mesh* makeMesh(Tile *tile);
	Mesh* makeMesh2(Tile *tile);

	float xScale = 0;
	float yScale = 0;
	float zScale = 0;
	worldView viewData;
	void makeMeshProcess(Tile* tile);
	void setMap();

	std::vector<DataPointInfo> findDataPoints(mzFloat mz, lcFloat lc, signalFloat sig);
	void clearAnnotations() { visibleAnnotations.clear();  annotations.clear(); annotationsLoaded = false;
	}

private:
	glm::vec2  get2d(float tx, float ty, float tz);

	GLuint peakTexture;
	const signalFloat peakUV = 0.995f;

	bool scalingValuesSet = false;
	void setScale(Tile* tile);

	Mesh* getJaggedMesh(Tile* tile);


	bool insertData(Tile* tile, std::vector<glm::vec3> &vertex_vec, std::vector<glm::vec2> &uv_vec, mzFloat x1, mzFloat x2, lcFloat y1, lcFloat y2, signalFloat z1, signalFloat z2, signalFloat z3, signalFloat z4);

	//takes a vb 
	bool inline insertData(Tile* tile, std::vector<glm::vec3> &vertex_vec, std::vector<glm::vec2> &uv_vec, std::vector<unsigned short> &vb_vec, std::vector<float> &attr_vec, mzFloat x1, mzFloat x2, lcFloat y1, lcFloat y2, signalFloat z1, signalFloat z2, signalFloat z3, signalFloat z4, byte mapVal, int alwaysDraw);

	//reuses points
	bool inline insertData(Tile* tile, std::vector<glm::vec3> &vertex_vec, std::vector<glm::vec2> &uv_vec, std::vector<unsigned short> &vb_vec, std::vector<float> &attr_vec, mzFloat x2, lcFloat y1, lcFloat y2, signalFloat z2, signalFloat z4, unsigned short p1, unsigned short p3, byte mapVal);
	//reuses 3 points
	bool inline insertData(Tile* tile, std::vector<glm::vec3> &vertex_vec, std::vector<glm::vec2> &uv_vec, std::vector<unsigned short> &vb_vec, std::vector<float> &attr_vec, mzFloat x2, lcFloat y2, signalFloat z4, unsigned short p1, unsigned short p3, unsigned short p2, byte mapVal);

	Camera* camera = NULL;
	std::vector<Annotation*> annotationsByName;
	std::vector<Annotation*> annotationsBestFirst;
	std::vector<std::string> info;


	void findDataPointMin(mzFloat mz, lcFloat lc, Tile* tile);

	void checkReBuild();
	bool readyToDrawFlag = false;
	std::vector<Tile*> loadedDataTiles;

	void manageQueue();


	std::vector<byte> serialiseTiles(Tile* tile);

	glm::mat4 transformMatrix;
	glm::ivec2 viewport;
	void reMake(Tile* tile);
	bool canDraw(Tile *tile);
	void draw(Tile* tile);

	void(*drawCallback)(Tile* tile,   bool isFaded) = NULL;
	void(*loadCallback)(Tile* tile) = NULL;
	void(*makeMeshCallback)(Tile* tile) = NULL;
	void prepareTile(Tile* tile);
	
	std::vector<std::vector<Label>> labels;

	std::vector<Tile*> tiles;
	std::vector<Tile*> new_tiles;
	std::mutex tileLock;
	inline glm::vec4  transform(glm::vec3 input);

	void addMarkers();
	std::vector< Annotation> annotations;
	bool annotationsLoaded = false;
	int sortedAnnotations = 0;
		

 
};



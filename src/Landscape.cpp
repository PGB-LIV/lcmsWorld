#include "Landscape.h"
#include "Tile.h"

#include <algorithm>
#include <ctime>
#include <ratio>
#include <chrono>
#include "Globals.h"
#include "Structs.h"
#include "Camera.h"
#include "SystemSetup.h"
#include "Render.h"
#include <list>

const static int TILES_PER_GB = 750;
const static int MIN_TILES = 4000;
static int tilesInRam = MIN_TILES;

static int GLtilesPerFrame = 50;
static int GLtilesThisFrame = 0;
static int loadTilesPerFrame = 100;
static int loadTilesThisFrame = 0;

const static double minSize = 0.005*2;

const double minSizePreload = 0.004*2;

const static double maxSize = 0.25;
const static double maxSizePrepare = 0.22;



Landscape::Landscape()
{
	viewData = { 3500,80000,30000 };
 
	tilesInRam = (int) (MIN_TILES + (System::systemMemory )*TILES_PER_GB);
 	std::cout << "capping memory at " << tilesInRam << " tiles \n";
	Range<mzFloat> t1;
	t1.min = mzFloat_max;
	t1.max = mzFloat_min;
	Range<lcFloat> t2;
	t2.min = lcFloat_max;
	t2.max = lcFloat_min;

	Range<signalFloat> t3;
	t3.min = signalFloat_max;
	t3.max = signalFloat_min;
 

	MZDataInfo defaultFilter = { worldMzRange,worldLcRange,worldSignalRange,0 };
	filter = defaultFilter;
	sortedAnnotations = 0;
}

void Landscape::updateWorldRange(MZDataInfo info)
{
 
	
	worldMzRange.min = std::min(worldMzRange.min, info.mzRange.min);
	worldMzRange.max = std::max(worldMzRange.max, info.mzRange.max);

 
	worldLcRange.min = std::min(worldLcRange.min, info.lcRange.min);
	worldLcRange.max = std::max(worldLcRange.max, info.lcRange.max);


	worldSignalRange.min = std::min(worldSignalRange.min, info.signalRange.min);
	worldSignalRange.max = std::max(worldSignalRange.max, info.signalRange.max);
	MZDataInfo defaultFilter = { worldMzRange,worldLcRange,worldSignalRange,0 };
	filter = defaultFilter;

}

 
static std::mutex annotationsLock;

void Landscape::addAnnotation(Annotation a) {
	
	if (a.shortText.length() < 1)
		a.shortText = a.text;

	if (a.shortText.length() > 60)
		a.shortText = a.shortText.substr(0, 60);


	annotationsLock.lock();
	annotations.push_back(a);
	annotationsLock.unlock();
}


static std::mutex rebuildLock;

//todo - check that this actually gets cleared
void Landscape::checkReBuild()
{
	
	if (!(filter == lastFilter))
	{
		lastFilter = filter;
		//todo - don't rebuild the wireframe, just the quads
		// Render::rebuildWireframe();
	}
	std::lock_guard<std::mutex> guard(rebuildLock);


	std::list<Tile*> remove;

	
	
		for (auto tile : rebuildQueue)
		{
			if (tile->drawStatus == DrawStatus::ready)
			{
				tile->clearMesh(true);
				remove.push_back(tile);
			}
			if (tile->drawStatus == DrawStatus::noData)
				remove.push_back(tile);

		}
	
		for (auto tile : remove)
		{
			rebuildQueue.erase(tile);

		}

	
}

void Landscape::reBuild()
{
	std::lock_guard<std::mutex> guard(rebuildLock);


	Render::rebuildWireframe();

	for (auto tile : tileList)
	{

#if 1
		//will try to leave the gl mesh in place
		if (tile->drawStatus == DrawStatus::ready)
			tile->clearMesh(true);
		else
		{
			if (tile->drawStatus != DrawStatus::noData)
			rebuildQueue.insert(tile);
		}
#else
		if (tile->drawStatus == DrawStatus::ready)
			tile->clearGLMesh(false);
		if (tile->drawStatus == DrawStatus::noGLMesh)
			tile->clearMesh(false);
#endif

	}

}
void Landscape::reMake(Tile* tile)
{
 //This can ONLY be used if we are loading on the fly
	return;

	tile->updateWorldRange();
	tile->clearGLMesh();
	tile->clearMesh();

	for (auto child : tile->getChildren())
		reMake(child);

}

void Landscape::reMake()
{
	for (auto tile : tiles)
		reMake(tile);
}

std::deque<Tile*> makeGLMeshQueue;

void Landscape::prepareTile(Tile* tile)
{
	
	double maxSize = maxSizePrepare;

 
	
	if (tile->LOD == 0)
	tile->setScreenSize(transformMatrix, viewport);


	// which children to prepare...
	
	
	//size will be -1 if tile can not be seen


	if ((tile->getScreenSize() > maxSize) || tile->LOD == 0)
	{
	 
		for (auto child : tile->getChildren())
		{
			child->setScreenSize(transformMatrix, viewport);

	//		 std::cout << child->getScreenSize() << " <? " << minSize << "   " << (int)tile->drawStatus << " \n";

			if ((child->getScreenSize() >= minSizePreload) )
			{
				 
				prepareTile(child);
			}
		}
	}
 
 

//	if (tile->LOD ==0)

	

	if (tile->drawStatus == DrawStatus::noData)
	{
 	if (loadCallback != NULL)
		{

 
				tile->drawStatus = DrawStatus::loadingData;

				loadCallback(tile);
 
		}

	}
	
		if (tile->drawStatus == DrawStatus::reCreateMesh)
		{
			//testing (should be clear, but just in case)
			tile->clearMesh();
			tile->drawStatus = DrawStatus::reCreatingMesh;
			makeMeshCallback(tile);
			makeGLMeshQueue.push_back(tile);


		}
	if (tile->drawStatus == DrawStatus::noMesh)
	{

		tile->drawStatus = DrawStatus::creatingMesh;
		makeMeshCallback(tile);
		makeGLMeshQueue.push_back(tile);
	}

	// Be aware that this may not always get called
	// i.e., a tile can be loaded, then go out of scope
	// unused tile cleanup should cope with this situation
	// (i.e., it looks like a memory leak, but isn't)

	if (tile->drawStatus == DrawStatus::noGLMesh)
	{
		if (GLtilesThisFrame-- > 0)
		{
			tile->drawStatus = DrawStatus::creatingGLMesh;
			tile->setGLMesh();
		}
	}
	if (tile->drawStatus == DrawStatus::reCreateGLMesh)
	{
		if (GLtilesThisFrame-- > 0)
		{
			tile->drawStatus = DrawStatus::reCreatingGLMesh;
			tile->setGLMesh(true);
		}
	}
}


void  Landscape::prepareTiles(const std::vector<Tile*> tiles)
{
	if (makeMeshCallback == NULL)
		return;

	GLtilesThisFrame = GLtilesPerFrame;
	loadTilesThisFrame = loadTilesPerFrame;
	//This meanst that all top-level tiles are prepated
	for (Tile* tile : tiles)
	{

		prepareTile(tile);
	}
}
void Landscape::updateCamera(float time)
{
	if (camera == NULL)
		return;

	camera->updateCamera(time);
 }

glm::mat4 Landscape::getCameraMatrix()
{
	if (camera == NULL)
		return glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	return camera->getViewMatrix();
}


Annotation Landscape::getClosestAnnotation(DataPoint cursor)
{
	Annotation closest;
	float mzSearch = 2.0f;
	float lcSearch = 5.0f;

	annotationsLock.lock();

	closest.mz = -10;
	if (annotations.size() == 0)
	{
		annotationsLock.unlock();
		return closest;
	}

 	float dist = (mzSearch*mzSearch)+ (lcSearch*lcSearch);

	Annotation find;
	find.mz = cursor.mz - mzSearch;
	find.lc = cursor.lc;
 
	auto low = std::lower_bound(annotations.begin(), annotations.end(), find);
 
	while (low < annotations.end())
	{
		auto a = *low;

		float dx = cursor.mz - a.mz;
		dx *= dx;
		
		float dy = cursor.lc - a.lc;
		dy *= dy;
		
		if (dx < mzSearch*mzSearch)
		if (dx + dy < dist)
		{
 
			closest = a;
			dist = dx + dy;

		}

		if (low->mz > cursor.mz + mzSearch)
			break;
		low++;
	}
 
	annotationsLock.unlock();
	return closest;
}
 
void  Landscape::drawAnnotations()
{
	int max_annotations = 999;


	int i = 0;
	for (auto ap : visibleAnnotations)
	{
		auto a = *ap;
		addMarker((float) a.mz, (float) a.lc,0, 2, a.shortText,a.width,a.height, Settings::cubeSize);
		a.isVisible = true;
		if (i++ > max_annotations)
			break;
	}
}
void Landscape::addMarkers()
{


	int xs = 20;
	int ys = 20;
	for (int x = 0; x < xs; x++)
	{
		auto xscale = (worldMzRange.max - worldMzRange.min);
		auto xstart = worldMzRange.min;
		float xpos = (float) ( xstart + (xscale*x) / xs);
		for (int y = 0; y < ys; y++)
		{
			auto yscale = (worldLcRange.max - worldLcRange.min);
			auto ystart = worldLcRange.min;
			float ypos = (float) (ystart + (yscale*y) / ys);
			
			if ((y & 1)==0)
			{
				addMarker(xpos, ypos, 0, 1, std::to_string((int)xpos),30,15);
			}
			else
			{
				
				addMarker(xpos, ypos, 0, 0, std::to_string((int)ypos), 30, 15);
			}


		}
	}
}


//Cube data
static const GLfloat g_vertex_buffer_data[] = {


-1.0f,-1.0f,-1.0f, // triangle 1 : begin
	-1.0f,-1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f, // triangle 1 : end

		-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	#if 1 // this is the bottom quad

	1.0f, 1.0f,-1.0f, // triangle 2 : begin
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f,-1.0f, // triangle 2 : end
				1.0f, 1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,

	#endif
		1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,

	1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,


	//ok
	1.0f, 1.0f, 1.0f,
1.0f, 1.0f, -1.0f,
-1.0f, 1.0f, -1.0f,

1.0f, 1.0f, 1.0f,
-1.0f, 1.0f, -1.0f,
-1.0f, 1.0f, 1.0f,


//tf
1.0f, 1.0f, 1.0f,
1.0f, -1.0f, -1.0f,
1.0f, 1.0f, -1.0f,




1.0f, -1.0f, -1.0f,
1.0f, 1.0f, 1.0f,
1.0f, -1.0f, 1.0f,




//tf
-1.0f, 1.0f, 1.0f,
-1.0f, -1.0f, 1.0f,
1.0f, -1.0f, 1.0f,

1.0f, 1.0f, 1.0f,
-1.0f, 1.0f, 1.0f,
1.0f, -1.0f, 1.0f,







};

std::vector<glm::vec3> vertex_vec;
std::vector<glm::vec2> uv_vec;
std::vector < GLMesh *> cubeMarkers;
void Landscape::add3dMarker(float tx, float ty, float tz, int type, std::string text, float width, float height, float cubeSize)
{
	

	tz += 0;
	for (int i = 0; i < sizeof(g_vertex_buffer_data) / sizeof(g_vertex_buffer_data[0]); i += 3)
	{
		float x, y, z;
		if (g_vertex_buffer_data[i+0] < 0)
			x = tx ;
		else
			x = tx + cubeSize / Settings::scale.x;

		if (g_vertex_buffer_data[i + 1] < 0)
			y = ty; //  -cubeSize / Settings::scale.y / 2;
		else
			y = ty + cubeSize / Settings::scale.y;

		if (g_vertex_buffer_data[i + 2] < 0)
			z = tz  ;
		else
			z = tz + cubeSize / Settings::scale.z;


		vertex_vec.push_back(glm::vec3(x, z, y));
		float dir = 0.25f;
		
			
		if (type == 1)
			dir = 0.75f;
 


		uv_vec.push_back(glm::vec2(dir, dir));

	}


}

void Landscape::drawCubes()
{
	static GLMesh*  cubeMesh = NULL;
 

	//this regenerates the meshes list every frame
 

	if (cubeMesh != NULL)
	{
		delete cubeMesh;
		cubeMesh = NULL;

	}

 

	if ((cubeMesh == NULL) && vertex_vec.size() > 0)
	{
		Mesh* newMesh = new Mesh(vertex_vec, uv_vec);
		cubeMesh = new GLMesh(newMesh, false);
		delete newMesh;
	}

	if (cubeMesh != NULL)
 		Render::drawCubeMesh(cubeMesh,0);
	

	cubeMarkers.clear();
	vertex_vec.clear();
	uv_vec.clear();

}

bool compareAlpha(Annotation* a, Annotation* b) { return (a->text < b->text); }
bool compareScore(Annotation* a, Annotation* b) { return (a->score > b->score); }

std::vector<Annotation> &Landscape::getAnnotations() {
	if (sortedAnnotations != annotations.size())
	{
		annotationsLock.lock();
		std::sort(annotations.begin(), annotations.end());
		annotationsLock.unlock();

		sortedAnnotations = annotations.size();

		annotationsByName.clear();
		annotationsByName.reserve(sortedAnnotations);
		annotationsBestFirst.clear();
		annotationsBestFirst.reserve(sortedAnnotations);
		for (unsigned int i = 0; i < annotations.size(); i++)
		{
			annotations[i].isVisible = false;
			annotationsByName.push_back(&annotations[i]);
			annotationsBestFirst.push_back(&annotations[i]);

		}

 
		std::sort(annotationsByName.begin(), annotationsByName.end(), compareAlpha);
		std::sort(annotationsBestFirst.begin(), annotationsBestFirst.end(), compareScore);

		visibleAnnotations.clear();
	}
	return annotations;
}



void Landscape::addMarker(float tx, float ty, float tz, int i, std::string text, float width, float height, float cubeSize)
{
	
		glm::vec4  mid = transform({ tx,ty,tz });

		
		if (cubeSize > 0)
		{
			add3dMarker(mid.x, mid.z, mid.y, i, text, width, height, cubeSize);
			add3dMarker(mid.x, mid.z, mid.y, i, text, width, height, 10);
		}

		auto m = transformMatrix * mid;
		m = m / (m.w);

		if (m.z < 1)
			if (m.z > 0.0001)
				if (std::abs(m.x) < 1.1)
					if (std::abs(m.y) < 1.1)
					{
						int label_x = (int)(((m.x + 1)*viewport.x / 2) - (width/2));


						int label_y = (int)(((-m.y + 1)*viewport.y / 2) - (height/2));
				 
						Label label = { label_x,label_y,text,0 };

							addLabel(i+1, label);
					}
	 
}

std::vector<float>  Landscape::getFilterModelSpace()
{
	glm::vec3 tl = { (float) filter.mzRange.min, (float)filter.lcRange.min,(float)filter.signalRange.min-1 };
	glm::vec3 br = { (float) filter.mzRange.max, (float)filter.lcRange.max,(float)filter.signalRange.min };
	
	auto tlt = transform(tl);
	auto brt = transform(br);

	std::vector<float> result = { tlt.x, brt.x, tlt.z, brt.z, brt.y };
	return result;

}
 

inline glm::vec4  Landscape::transform(glm::vec3 input)
{
	//todo
	//owner dereferencing should be removed, and values cached

	auto y1 = input.y;
	glm::vec4  result;
	y1 -= (float) (worldLcRange.min + worldLcRange.max) / 2;

	y1 *= yScale;

	auto x1 = input.x;
	x1 -= (float)(worldMzRange.min + worldMzRange.max) / 2;
	x1 *= xScale;

	auto z1 = input.z;
	z1 *= zScale;
	//then convert them - e.g., function could be logarithmic
	//note it is already normalised into 3d world space, which is current undefined
	z1 = Mesh::convertZ(z1);
	result.x = x1;
	result.z = y1;
	result.y = z1;


	result.w = 1;
	return result;

}



void Landscape::draw(Tile* tile)
{
	checkReBuild();


	if (tile->mzRange.max < filter.mzRange.min)
		return;
	if (tile->mzRange.min > filter.mzRange.max)
		return;


	if (tile->lcRange.max < filter.lcRange.min)
		return;
	if (tile->lcRange.min > filter.lcRange.max)
		return;


		auto children = tile->getChildren();

		int visChildren = 0;
		int readyChildren = 0;

		if ((tile->getScreenSize() > maxSize)  )
		for (auto child : children)
		{


			if ((child->getScreenSize() > minSize)  )
			{
				visChildren++;


				if (canDraw(child))
					readyChildren++;

 
					
			}

		}

		bool drawChildren = false;
		if (visChildren > 0)
		{
			if ((Globals::currentTime.time - tile->childTime.time) < 1e6)
			{
				drawChildren = true;

			}
		}
		if ((visChildren > 0) && (readyChildren >= visChildren))
		{
			tile->childTime = Globals::currentTime;
				drawChildren = true;

		}

		//we can definitely draw children instead
		if (drawChildren)
		{
			tile->childTime = Globals::currentTime;
			for (auto child : children)
			{
				draw(child);
			}

		//	if (tile->isOnScreen())
			//should only draw here if fading out
			if ((tile->drawStatus == DrawStatus::ready)
				|| (tile->drawStatus == DrawStatus::reCreatingMesh)
				|| (tile->drawStatus == DrawStatus::reCreateMesh)
				|| (tile->drawStatus == DrawStatus::reCreateGLMesh)
				|| (tile->drawStatus == DrawStatus::reCreatingGLMesh))
			{
 
				drawCallback(tile, true);
			

			}
  		return;
		}



		//	if (tile->getScreenSize() > 0.01)
		//if (tile->isOnScreen())
		if ((tile->drawStatus == DrawStatus::ready)
			|| (tile->drawStatus == DrawStatus::reCreatingMesh)
			|| (tile->drawStatus == DrawStatus::reCreateMesh)
			|| (tile->drawStatus == DrawStatus::reCreateGLMesh)
			|| (tile->drawStatus == DrawStatus::reCreatingGLMesh))
		{
		 
//			std::cout << (int) tile->drawStatus << "  " << tile->getScreenSize() <<  "   " << tile->id <<"  : " << tile->LOD <<"\n";
			drawCallback(tile, false);
			tileDrawn(tile);
		}

 

}

GLMesh* Landscape::makeBaseQuads(GLuint textureId)
{
	

	if (readyToDrawFlag == false)
		return NULL;
	int height = (int) (worldLcRange.max - worldLcRange.min);


	if ((height == 0) || (xScale == 0) )
		return NULL;

	int num_vertices = 6;

	std::vector<glm::vec3> vertex_vec;
	vertex_vec.reserve(num_vertices);

	std::vector<glm::vec2> uv_vec;
	uv_vec.reserve(num_vertices);
	auto mzRange = worldMzRange;
	auto lcRange = worldLcRange;
	
	auto fmzRange = filter.mzRange;
	auto flcRange = filter.lcRange;

	mzFloat x1 = fmzRange.min;
	mzFloat x2 = fmzRange.max;


	x1 -= (mzRange.min + mzRange.max) / 2;
	x2 -= (mzRange.min + mzRange.max) / 2;


	x1 *= (mzFloat)xScale;
	x2 *= (mzFloat)xScale;



	lcFloat y1 = flcRange.min;
	y1 -= (lcRange.min + lcRange.max) / 2;
	y1 *= (mzFloat)yScale;
	lcFloat y2 = flcRange.max;
	y2 -= (lcRange.min + lcRange.max) / 2;
	y2 *= (mzFloat)yScale;


 
 
	float z = 0;
	//points in Z shape
	glm::vec3 p1 = glm::vec3(x1, z, y1);
	glm::vec3 p2 = glm::vec3(x2, z, y1);
	glm::vec3 p3 = glm::vec3(x2, z, y2);
	glm::vec3 p4 = glm::vec3(x1, z, y2);

	vertex_vec.push_back(p1);
	vertex_vec.push_back(p2);
	vertex_vec.push_back(p3);

	vertex_vec.push_back(p1);
	vertex_vec.push_back(p3);
	vertex_vec.push_back(p4);
	


	//uvs
	glm::vec2 c = glm::vec2(0.25, zOffset);
	

	uv_vec.push_back(c);
	uv_vec.push_back(c);
	uv_vec.push_back(c);
	uv_vec.push_back(c);
	uv_vec.push_back(c);
	uv_vec.push_back(c);

	Mesh* mesh = new Mesh(vertex_vec, uv_vec);
	mesh->setTexture(textureId);
	auto wire = new GLMesh(mesh, false);

	delete mesh;
	return wire;
}

GLMesh* Landscape::makeBaseMesh(GLuint textureId)
{
 
	

	if (readyToDrawFlag == false)
		return NULL;

	int width = (int) (filter.mzRange.max - filter.mzRange.min) / 10;



	int height = (int) (filter.lcRange.max - filter.lcRange.min) ;

	
	if (height == 0)
		return NULL;

	//todo - this is not a good way of telling we are ready to draw
	if (xScale == 0)
	return NULL;
	int num_vertices = (width + height) * 2;

	std::vector<glm::vec3> vertex_vec;
	vertex_vec.reserve(num_vertices);

	std::vector<glm::vec2> uv_vec;
	uv_vec.reserve(num_vertices);

	auto mzRange = worldMzRange;
	auto lcRange = worldLcRange;

	auto mzRangef = filter.mzRange;
	auto lcRangef = filter.lcRange;
	float z = 0;

	for (int i = 0; i < height; i++)
	{
		float x1 = (float)mzRangef.min;
		float x2 = (float)mzRangef.max;

		x1 -= (float)(mzRange.min + mzRange.max) / 2;
		x2 -= (float)(mzRange.min + mzRange.max) / 2;
 

		x1 *= (float)xScale;
		x2 *= (float)xScale;
 


		float y = (float)  ((((lcRangef.max - lcRangef.min)*i) / height) + lcRangef.min);
		y -= (float)(lcRange.min + lcRange.max) / 2;

		y *= (float)yScale;
		
 
		
		//points in Z shape
		glm::vec3 p1 = glm::vec3(x1, z, y);
		glm::vec3 p2 = glm::vec3(x2, z, y);

		//push the triangles onto the vertices




		vertex_vec.push_back(p1);
		vertex_vec.push_back(p2);


		//uvs
		glm::vec2 c1 = glm::vec2(0, 0);
		glm::vec2 c2 = glm::vec2(0, 0);

		uv_vec.push_back(c1);
		uv_vec.push_back(c2);
	}

	

	for (int i = 0; i < width; i++)
	{
		float y1 = (float)lcRangef.min;
		float y2 = (float)lcRangef.max;

		y1 -= (float)(lcRange.min + lcRange.max) / 2;
		y2 -= (float)(lcRange.min + lcRange.max) / 2;


		y1 *= (float)yScale;
		y2 *= (float)yScale;



		float x = (((mzRangef.max - mzRangef.min)*i) / width) + mzRangef.min;
		x -= (float)(mzRange.min + mzRange.max) / 2;

		x *= (float)xScale;

 

		//points in Z shape
		glm::vec3 p1 = glm::vec3(x, z, y1);
		glm::vec3 p2 = glm::vec3(x, z, y2);

		//push the triangles onto the vertices




		vertex_vec.push_back(p1);
		vertex_vec.push_back(p2);


		//uvs
		glm::vec2 c1 = glm::vec2(0, 0);
		glm::vec2 c2 = glm::vec2(0, 0);

		uv_vec.push_back(c1);
		uv_vec.push_back(c2);
	}

	assert(vertex_vec.size() == num_vertices);
 
	Mesh* mesh = new Mesh(vertex_vec, uv_vec);
	mesh->setTexture(textureId);
	auto wire = new GLMesh(mesh, false);

	delete mesh;
	return wire;
}

 
void Landscape::dataLoaded(Tile* tile)
{
	loadedDataTiles.push_back(tile);
	 
}

void Landscape::manageQueue()
{

	if (loadedDataTiles.size() < 5)
		return;

 

	std::vector<Tile*> notReady;


	if (loadedDataTiles.size() > tilesInRam) // tilesInRam)
	{
 
		std::sort(loadedDataTiles.begin(), loadedDataTiles.end(), Tile::compareTilePtrReverse);
	 
		int numToClear = 90;
 		while (loadedDataTiles.size() > tilesInRam - numToClear)
		{

	 
			
			Tile* next = loadedDataTiles.back();

			if ((Globals::currentTime.time - next->lastLoaded.time) < 5e6)
			{
				numToClear++;

				notReady.push_back(next);
				loadedDataTiles.pop_back();
				continue;

			}

			if (next->unLoad() == false)
			{
				numToClear++;
				if (numToClear > 300)
					numToClear = 300;
				notReady.push_back(next);

			}
			loadedDataTiles.pop_back();

		}
	//	for (auto t : notReady)
	//		loadedDataTiles.push_back(t);

	}

	
}
inline void Landscape::tileDrawn(Tile* tile)
{
	//is actually being drawn (but may be offscreen)
	tile->beingDrawn();
 
}

inline bool Landscape::canDraw(Tile *tile)
{
	auto children = tile->getChildren();

	int visChildren = 0;
	int readyChildren = 0;

	//only check chilren if this is quite large
	//if ((tile->getScreenSize() > maxSize) )
	for (auto child : children)
	{
		 
		if ((child->getScreenSize() > minSize)  )
		{
			visChildren++;
			if (canDraw(child))
				readyChildren++;
		}

	}

	//we can definitely draw children instead
	if ((visChildren > 0) && (visChildren == readyChildren))
		return true;


//  	if (tile->getScreenSize() > 0.001)
	if ((tile->drawStatus == DrawStatus::ready)
		|| (tile->drawStatus == DrawStatus::reCreatingMesh)
		|| (tile->drawStatus == DrawStatus::reCreateMesh)
		|| (tile->drawStatus == DrawStatus::reCreateGLMesh)
		|| (tile->drawStatus == DrawStatus::reCreatingGLMesh))
	{
			return true;
	}
	
	return false;
}


 void Landscape::drawTiles(const std::vector<Tile*> draw_tiles)
{


	Globals::setCurrentTime();
 
	 

	if (draw_tiles.size() < 1)
		return;
	if (drawCallback == NULL)
		return;
	
 
	if (1)
	{
		bool all_drawn = true;
		int cnt = 0;
		int cd = 0;

		for (Tile* tile : draw_tiles)
		{
			int ds = (int)tile->drawStatus;
			cnt++;


			//		drawCallback(tile);
			//		continue;

			if (canDraw(tile))
			{
				cd++;
				draw(tile);
			}


		}

		Render::drawDeferred();
		Render::drawTarget();
		manageQueue();
		drawCubes();

	}

	static GLMesh* targetMesh = NULL;


	//this regenerates the meshes list every frame

 

	if ((targetMesh == NULL) )
	{

//		
	 	static const GLfloat g_vertex_buffer_data[] = { 0,0,0,  0,0,1,  1,0,0 , 0,0,1,   1,0,0,  1,0,1 };

		std::vector<glm::vec3> vertex_vec;
		std::vector<glm::vec2> uv_vec;
		
		for (int i = 0; i < sizeof(g_vertex_buffer_data) / sizeof(g_vertex_buffer_data[0]); i += 3)
		{
			float x, y, z;
			x = (g_vertex_buffer_data[i + 0]-.5 ) * -200;
			y = (g_vertex_buffer_data[i + 1] ) * 200;
			z = (g_vertex_buffer_data[i + 2] - .5) * 200;
			vertex_vec.push_back(glm::vec3(x, y, z));
			uv_vec.push_back(glm::vec2(-g_vertex_buffer_data[i + 0], -g_vertex_buffer_data[ i + 2]));
		}


		Mesh* newMesh = new Mesh(vertex_vec, uv_vec);
		targetMesh = new GLMesh(newMesh, false);

		delete newMesh;
	}

	if (targetMesh != NULL)
		Render::drawCubeMeshDirection(targetMesh, 0);


}

void Landscape::updateViewport(int x, int y)
{
	
	viewport.x = x;
	viewport.y = y;
}

void Landscape::addPendingTiles()
{
	if (new_tiles.size() < 1)
		return;

	//adding new tiles is to buffer to prevent delays due to thread safety

	tileLock.lock();

	for (Tile* newTile : new_tiles)
	{
		tiles.push_back(newTile);
	}
	new_tiles.clear();

	tileLock.unlock();
}
void Landscape::updateLandscape(glm::mat4 matrix)
{
	transformMatrix = matrix;

	//todo tidy up markers  /vbo etc.
	//markers.clear();

		
	if (Settings::showNumbers)
	if (readyToDrawFlag)
		addMarkers();

 	prepareTiles(tiles);

	
	addPendingTiles();

	return;

}

void Landscape::drawTiles()
{
	drawTiles(tiles);

 }

 

std::vector<byte> Landscape::serialiseTiles(Tile* tile)
{
 
	std::vector<byte>  buffer = tile->serialise();
	
 
	auto children = tile->getChildren();
//	std::cout << children.size() << "  ";
	for (auto child : children)
	{
		auto childBuffer = serialiseTiles(child);
		// buffer.insert(buffer.end(), childBuffer.begin(), childBuffer.end());

	}
	return buffer;
}
std::mutex listLock;
void Landscape::registerTile(Tile* t) { 
	listLock.lock();
	tileList.push_back(t);
	listLock.unlock();
}


std::vector<byte> Landscape::serialiseData()
{
	std::vector<byte> buffer;
	int ptr = 0;

	buffer.resize(1024 * 1024 *16);
	
	int ver = 87654321;

	memcpy(&buffer[ptr], &ver, sizeof(ver));
	ptr += sizeof(ver);



 	memcpy(&buffer[ptr], &worldLcRange, sizeof(worldLcRange));
	ptr += sizeof(worldLcRange);

 	memcpy(&buffer[ptr], &worldMzRange, sizeof(worldMzRange));
	ptr += sizeof(worldMzRange);
 	memcpy(&buffer[ptr], &worldSignalRange, sizeof(worldSignalRange));
	ptr += sizeof(worldSignalRange);

	//todo - urgent - fix numTiles - this is only the number of top-level tiles

	int numTiles = (int) tileList.size();
	memcpy(&buffer[ptr], &numTiles, sizeof(numTiles));
	ptr += sizeof(numTiles);

	for (auto tile : tileList)
	{
		ptr += tile->serialise(&buffer[ptr]);
		if (ptr > buffer.size() *3/4)
			buffer.resize(buffer.size()*4/3);
	}
	buffer.resize(ptr);

	camera = new Camera(this);

	return buffer;
}

void tileInfo(Tile* tile, std::string par)
{
	auto mzRange = tile -> mzRange;
	auto lcRange = tile->lcRange;
	par = par + "  " + std::to_string(tile->id) + ". ";

	return;

	std::cout << tile->LOD << " , " << tile->id << " , " << mzRange.min  << "  " << par << "\n";
	for (auto child : tile->getChildren())
	{
		tileInfo(child,par);
		auto mzRange_c = child->mzRange;
		auto lcRange_c = child->lcRange;
		if (lcRange_c.min < lcRange.min)
			std::cout << " error lc min " << tile->id << " , " << child->id << "\n";
		if (lcRange_c.max > lcRange.max)
			std::cout << " error lc max " << tile->id << " , " << child->id << "\n";

		if (mzRange_c.min < mzRange.min)
			std::cout << " error mz min " << tile->id << " , " << child->id << "\n";

		if (mzRange_c.max > mzRange.max)
			std::cout << " error mz max " << tile->id << " , " << child->id << "\n";

	}
	
}
void Landscape::deSerialiseData(std::vector<byte> buffer, int viewLod)
{
	int ptr = 0;

	int ver = 87654321;

	memcpy(&ver, &buffer[ptr], sizeof(ver));
	ptr += sizeof(ver);

	std::cout << " Landscape ver = " << ver << "\n";

 

 	memcpy( &worldLcRange, &buffer[ptr], sizeof(worldLcRange));
	ptr += sizeof(worldLcRange);

 	memcpy( &worldMzRange, &buffer[ptr], sizeof(worldMzRange));
	ptr += sizeof(worldMzRange);

 	memcpy( &worldSignalRange, &buffer[ptr], sizeof(worldSignalRange));
	ptr += sizeof(worldSignalRange);

	int numTiles = 0;
 	memcpy(&numTiles, &buffer[ptr], sizeof(numTiles));
	ptr += sizeof(numTiles);

	std::cout << " Landscape range = " << worldSignalRange.max << "\n" << std::flush;

	for (int i=0; i < numTiles; i++)
	{
		Tile* tile = new Tile(this);
		int size = tile->deserialise(&buffer[ptr]);
		ptr += size;
 
		tileList.push_back(tile);
		if (tile->LOD == viewLod)
		
			addTile(tile);
	}
	Tile::setChildren();
	addPendingTiles();

	for (auto tile : tiles)
		tileInfo(tile, "");
	MZDataInfo info = { worldMzRange,worldLcRange,worldSignalRange, 1};
	std::cout << " Landscape range = " << worldSignalRange.max << "\n" << std::flush;

	updateWorldRange( info);
	camera = new Camera(this);

}

void Landscape::addTile(Tile* newTile) { 

	tileLock.lock();

	new_tiles.push_back(newTile); 

	tileLock.unlock();
}



void Landscape::findDataPointMin(mzFloat mz, lcFloat lc, Tile* tile)
{

}


std::vector<DataPointInfo> Landscape::findDataPoints(mzFloat mz, lcFloat lc, signalFloat sig)
{
	std::deque<Tile*> q;
	for (auto tile : tiles)
		q.push_back(tile);

	std::vector<DataPointInfo> found;
	while (q.size() > 0)
	{
		Tile* tile = q.front();
		q.pop_front();
		if (mz >= tile->mzRange.min)
			if (mz <= tile->mzRange.max)
				if (lc >= tile->lcRange.min)
					if (lc >= tile->lcRange.min)
					{
							DataPoint s = { mz,lc, -sig };
							std::vector<DataPointInfo> res = tile->findClosest(s);
							if (tile->type == MZDataType::jagged)
							{

								for (auto f : res)
								{
									if (f.signal >= 0)
										found.push_back(f);
								}
							}
							for (auto child : tile->getChildren())
							{
								if (child->hasData())
									q.push_back(child);

							}
						
					}
	}

	return found;
}

DataPoint Landscape::findDataPoint(mzFloat mz, lcFloat lc, signalFloat sig)
{
	std::deque<Tile*> q;
	for (auto tile : tiles)
		q.push_back(tile);

	std::vector<DataPointInfo> found;
	while (q.size() > 0)
	{
		Tile* tile = q.front();
		q.pop_front();
		if (mz >= tile->mzRange.min)
			if (mz <= tile->mzRange.max)
				if (lc >= tile->lcRange.min)
					if (lc >= tile->lcRange.min)
					{
						DataPoint s = { mz,lc, -sig };
						std::vector<DataPointInfo> res = tile->findClosest(s);

						for (auto f : res)
						{
 								if (f.signal >= 0)
									found.push_back(f);
						}
						
						for (auto child : tile->getChildren())
						{
							if (child->hasData())
								q.push_back(child);

						}
					}
		}

	
	if (found.size() == 0)
	{
		DataPoint p = { mz,lc,-1 };
		return p;
	}
	auto best = found[0];
	best.LOD = -1;
	//found contains the closest data point in each tile (that is being drawn)
	//look for one with highest LOD

	double closestDistance = std::numeric_limits<double>::max();
	 
 	DataPoint s = { mz,lc, std::max(sig ,0.0f )};

	for (auto p : found)
	{
//		if (p.LOD >= best.LOD)
		{
			double dx = p.mz - s.mz;
			double dy = p.lc - s.lc;

			dx *= xScale;
			dy *= yScale;

			double dz = p.signal - s.signal; //  (dx*dx) + (dy*dy);
			dz *= zScale;
			dz = Mesh::convertZ((signalFloat) dz);

			dx *= Settings::scale.x;
			dy *= Settings::scale.y;
			dz *= Settings::scale.z;
 
			

			double dist = std::sqrt((dx*dx) + (dy*dy));
			dist = dist * dist +(dz*dz);

			dist /= p.LOD +1;
			if (dist < closestDistance)
			{
				closestDistance = dist;
				best = p;
			}
		}
	}

 
	// std::cout << dx << " , " << dy << ", " << dz << "\n";
 	DataPoint p = { best.mz,best.lc,best.signal };
 

	return p;
	
}



Landscape::~Landscape()
{
	for (auto tile : tiles)
	{
		tile->updateWorldRange();
		tile->clearGLMesh();
		tile->clearMesh();
		delete tile;
	}


}
void Landscape::setDrawCallback(void(*callback)(Tile* mesh, bool isFading))
{
	drawCallback = callback;
}
void Landscape::setMakeMeshCallback(void(*callback)(Tile* mesh))
{
	makeMeshCallback = callback;
}

void Landscape::setLoadCallback(void(*callback)(Tile* mesh))
{
	loadCallback = callback;
}


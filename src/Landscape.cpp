//also need to change in Render.cpp
//#define THREADED 0 

#include "Landscape.h"
#include "Tile.h"

#include <algorithm>
#include <ctime>
#include <ratio>
#include <chrono>
#include <thread>
#include "Globals.h"
#include "Structs.h"
#include "Camera.h"
#include "SystemSetup.h"
#include "Render.h"
#include <list>
#include <sstream>
#include <iomanip>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp" // after <glm/glm.hpp>

 
//#define DISABLE_UNLOADING
const static int maxGB = 12;

const static int TILES_PER_GB = 350;
const static int MIN_TILES = 2500;
static int tilesInRam = MIN_TILES;

static int GLtilesPerFrame = 500;
static int GLtilesThisFrame = 0;
static int loadTilesPerFrame = 2500;
static int loadTilesThisFrame = 0;
//at what size to bother drawing child
//if it's smaller than this, it will be just left empty
const static double minSize = 0.005*2     *4 ;
const double minSizePreload = 0.002*2  * 1.5;
const double min_draw_size =  0.0001 * 1 ;
const static double maxSize = 0.25 * 2 * 1;
const static double maxSizePrepare = 0.24 * 2 *1;



Landscape::Landscape()
{
	//defines aspect ratio of default view
#if TOC_VERSION
	viewData = { 3500,40000*4,40000 };
#else
	viewData = { 3500,40000 * 16/9,40000 };
#endif
 
	int gb = std::min((int) (System::systemMemory+.5), maxGB);

	tilesInRam = (int) (MIN_TILES +gb*TILES_PER_GB);
	//tilesInRam = std::min(tilesInRam, TILES_PER_GB * 3);



	/// <summary>
#ifdef DISABLE_UNLOADING
	tilesInRam = 2 << 30;
#endif
 	std::cout << "capping memory at " << tilesInRam << " tiles for "<<gb<<" GB  \n";
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

	std::lock_guard<std::mutex> guard(annotationsLock);


	annotations.push_back(a);

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
	//if this one is quite big, prepare all of its children (as long sas they can be seen)

	if ((tile->getScreenSize() > maxSize) || tile->LOD == 0)
	{
	 

		for (auto child : tile->getChildren())
		{
			child->setScreenSize(transformMatrix, viewport);
		//	std::cout << " set size " << child->LOD << "  - " << child->getScreenSize() << "\n";

		}

		//test of setting each tile to the max of its siblings
		//so that all are drawn together, preventing missing tiles from breaking the rendering
		if (0)
		for (auto child : tile->getChildren())
		{
			child->setScreenSizeMaxSibling(tile->getChildren());

		}



		
		for (auto child : tile->getChildren())
		{
			// child->setScreenSize(transformMatrix, viewport);

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

		if (loadTilesThisFrame-- < 0)
			return;

 
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

	std::lock_guard<std::mutex> guard(annotationsLock);


	closest.mz = -10;
	if (annotations.size() == 0)
	{
 
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
 
 
	return closest;
}
 
void  Landscape::drawAnnotations()
{
	int max_annotations = 999;

	std::lock_guard<std::mutex> guard(annotationsLock);

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


glm::vec2  Landscape::get2d(float tx, float ty, float tz)
{


	glm::vec4  mid = transform({ tx,ty,tz });

 


	glm::vec2 ret = glm::vec2(-100, -100);

	auto m = transformMatrix * mid;
	m = m / (m.w);

	if (m.z < 1)
		if (m.z > 0.0001)
			if (std::abs(m.x) < 1.1)
				if (std::abs(m.y) < 1.1)
				{
					int label_x = (int)(((m.x + 1) * viewport.x / 2)  );


					int label_y = (int)(((-m.y + 1) * viewport.y / 2)  );
 

					ret = glm::vec2(label_x, label_y);

				}
	return ret;

}


DataPoint get3dPos(glm::vec3 cursor3d, Landscape*l)
{
float posX, posY, posZ;

float maxZ = .9999999999f;
float minZ = .6f;
DataPoint cursorPoint;
auto vp = l->getViewport();

int lp = 0;
std::cout << " lc get3d called\n";


do
{



	glm::vec4 vp4 = glm::vec4(0.0, 0.0, (GLdouble)vp.x, (GLdouble)vp.y);
	float zScale = Settings::scale.z;


	auto 	sm = glm::scale(glm::vec3(Settings::scale.x, zScale, Settings::scale.y));

	auto xyz = glm::unProject(cursor3d, sm, Render::ProjectionMatrix * Render::ViewMatrix, vp4);

	posX = xyz.x;
	posY = xyz.z;
	posX /= l->xScale;
	posX += (l->worldMzRange.min + l->worldMzRange.max) / 2;

	//if (Globals::neg_y)
//		posY = -posY;


	posY /= l->yScale;
	posY += (l->worldLcRange.min + l->worldLcRange.max) / 2;
//	posY += (l->worldSignalRange.min + l->worldSignalRange.max) / 2;

	posZ = xyz.y;
	posZ /= l->zScale;
	posZ += (l->worldLcRange.min + l->worldLcRange.max) / 2;
	//posZ += (l->worldSignalRange.min + l->worldSignalRange.max) / 2;
	//std::cout << posX << " , " << posY << " , " << posZ << "\n";

	cursorPoint.mz = posX;
	cursorPoint.lc = posY;
	cursorPoint.signal = posZ;


	if (cursorPoint.signal < 0)
	{
		maxZ = cursor3d.z;
		cursor3d.z = (cursor3d.z + minZ) / 2;
	}

	else
	{
		minZ = cursor3d.z;
		cursor3d.z = (cursor3d.z + maxZ) / 2;
	}

	//move closer to the camera if getting a -ve signal position
	lp++;

 } while ( ((cursorPoint.signal < 0) || (cursorPoint.signal > 100)) && (lp < 32));


return cursorPoint;


}


 
void Landscape::addMarkers()
{

	Camera* c =getCamera();
	auto centre_mz = c->currentTarget.mz;
	auto centre_lc = c->currentTarget.lc;
 

	auto dist = c->distance;

	//how many there are depends on how far from the camera
	int xs = (int) ((worldMzRange.max - worldMzRange.min) / dist ) * 2 ;

	int ys = (int)((worldLcRange.max - worldLcRange.min) / dist ) * 2;
	auto xscale = (worldMzRange.max - worldMzRange.min);

 
	auto yscale = (worldLcRange.max - worldLcRange.min);
 

	//don't draw too close  together (possible due to perspective)
	float screen_gap = 20;
	float screen_size = Globals::windowWidthActive;
	//move it slightly so that it doesn't cover the centre of the screen
//	glm::vec3 screenpos = glm::vec3(Globals::windowWidthActive *2/5, Settings::windowHeight *3/ 5,1/100000);
	
	glm::vec3 screenpos = glm::vec3(Globals::windowWidthActive * 2 / 5, Settings::windowHeight * 3 / 5, 1000  );

	//auto data = get3dPos(screenpos, this);
	
 

	centre_mz -= xscale* dist * .000001 / Settings::scale.x;
	centre_lc -= yscale* dist * .000005 / Settings::scale.y;

	if (centre_lc < worldLcRange.min)
		centre_lc = worldLcRange.min;
	if (centre_lc > worldLcRange.max)
		centre_lc = worldLcRange.max;

	if (centre_mz < worldMzRange.min)
		centre_mz = worldMzRange.min;
	if (centre_mz > worldMzRange.max)
		centre_mz = worldMzRange.max;
//	centre_lc = data.lc;
//	centre_mz = data.mz;
	//  std::cout << xs << "  " << xscale * 2 << "  " << (xs == xscale * 2) << "\n";
	// std::cout << xscale <<"  " << xs << "\n";

 
	double xsc = xs / xscale ;

	glm::vec2 last_pos = glm::vec2(-100, -100);


	float xranges[] = {.001, .01,.1, .5,1,2,5,10,20,50,100, 200, 400, 1000};
	float yranges[] = {.1, .5,1,2,5,10,20,50,100, 200, 400, 1000 };
//	float yranges[] = { .1, .2, .5,1,2,5,10,20,30,60 };

	 


	auto screen1 = get2d(centre_mz, centre_lc, 0);
	auto screen2 = get2d(centre_mz, centre_lc, 0);



	int u = 0;
	float xrange = 1;
	do
	{
		float ypos = centre_lc;
		xrange = xranges[u];
		float x1 = centre_mz - xrange / 2;
		float x2 = centre_mz + xrange / 2;
		screen1 = get2d(x1, ypos, 0);
		screen2 = get2d(x2, ypos, 0);
		u = u + 1;
		
	} while (glm::distance(screen1, screen2) < (screen_gap*4) && (u < sizeof(xranges)/sizeof(float)));
	
	u = 0;
	float yrange = 1;
	do
	{
		
		yrange = yranges[u];
		float xpos = centre_mz ;
		float y1 = centre_lc - yrange / 2;
		float y2 = centre_lc + yrange / 2;
		
		screen1 = get2d(xpos, y1, 0);
		screen2 = get2d(xpos, y2, 0);
		u = u + 1;

	} while (glm::distance(screen1, screen2) < (screen_gap*2  ) && (u < sizeof(yranges) / sizeof(float)));




	float xstart = centre_mz;
 
	auto centreyp = c->currentTarget.lc;;
	auto centre = get2d(c->currentTarget.mz, centreyp, 0);

 
	{
		float dist = 0.001;


		while (true)

		{
			dist = dist * 2;
			auto screen = get2d(c->currentTarget.mz - dist, centreyp, 0);
			
 
 
			if (screen.x < -99)
				break;

			if (glm::distance(screen, centre) > screen_size  )
				break;
			if (dist > worldMzRange.max)
				break;
		}
 		xstart = xstart - dist;
		if (xstart < worldMzRange.min)
			xstart = worldMzRange.min;
	}

	
	xstart = ((int)(xstart / xrange + 0.5)) * xrange;
	//std::cout << xstart << "   " << xrange << "\n";

	bool drawing = false;
	for (float xpos = xstart; xpos < worldMzRange.max; xpos += xrange)
	{

	 
		float ypos = centre_lc;;

		auto screen = get2d(xpos, ypos, 0);

		
			if (glm::distance(screen, centre) > screen_size)
			{
				if (drawing)
					break;
				continue;
			}


		if (glm::distance(screen, last_pos) < screen_gap * 2)
		{

			continue;
		}
		drawing = true;
		last_pos = screen;
		
		std::stringstream stream;
		stream <<  xpos;
		std::string s = stream.str();


		 		std::string text = std::to_string((int)(xpos + .49));
				text = s;

		 
					addMarker(xpos, ypos, 0, 1, text, 30, 15);
		 

	}

 
 
		double ysc = ys / yscale * 400;
 
 
 

	last_pos = glm::vec2(-100, -100);

	float ystart = (float) round(worldLcRange.min);
	ystart = ((int)(ystart / yrange)) * yrange;

	for (float ypos = ystart; ypos < worldLcRange.max; ypos += yrange)
	{

		
		float xpos = centre_mz;
 
 

		auto screen = get2d(xpos, ypos, 0);
		
		if (glm::distance(screen, last_pos) < screen_gap)
			continue;

		last_pos = screen;

		float yd = (float)(ystart + (yscale * 1) / ys);
		std::stringstream stream;
 
		int dp = 0;
		if (yrange < 1)
			dp = 1;

		stream << std::fixed << std::setprecision(dp) << ypos;
		std::string label = stream.str();

 
			addMarker(xpos, ypos, 0, 0, label, 30, 15);
	 


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

void Landscape::addDSquare(float tx, float ty, float tz, float size, float sizey)
{

	glm::vec4  mid = transform({ tx,ty,tz });

 

	float u = 0.75f;
	float v = 0.25f;
 
	float xoffs[] = { 0, 1, -1,   0, 2, 1 };
	float yoffs[] = { 1, 0, 0,    1,1, 0 };

	
	for (int i = 0; i < 6; i++)
	{
		int j = i;
		auto x = mid.x + ( xoffs[j])*size;
		auto y = mid.z + yoffs[j] * sizey;
		
		auto z = mid.y ;

 		vertex_vec.push_back(glm::vec3(x, z, y));
		uv_vec.push_back(glm::vec2(u, v));
	 
	}

}

void Landscape::addSquare(float tx, float ty, float tz, float size, float sizey, int col)
{

	glm::vec4  mid = transform({ tx,ty,tz });

	float ucols[] = { .05,.75,.95,.05,.25,.25,.95,.75,             .75,.95,.25,.25, .05, .95, .75, .05 };
	float vcols[] = { .05,.25,.05,.95,.75,.25,.95,.75    ,      .75,.95,.25,.75, .95, .05, .25, .05 };

	if (!Settings::colouredGridLines)
		col = 2;

	float u = ucols[col & 7];
	float v = vcols[col & 7];
	
	

	float xoffs[] = { 0, 1, 0,    0, 1, 1 };
	float yoffs[] = { 1, 0, 0,    1, 1, 0 };


	for (int i = 0; i < 6; i++)
	{
		int j = i;
		auto x = mid.x + xoffs[j] * size;
		auto y = mid.z + yoffs[j] * sizey;

		auto z = mid.y;
	//	if (y > worldLcRange.max)
	//		y = worldLcRange.max;

		vertex_vec.push_back(glm::vec3(x, z, y));
		uv_vec.push_back(glm::vec2(u, v));

	}

}



void Landscape::add3dMarker(float tx, float ty, float tz, int type, std::string text, float width, float height, float cubeSize)
{
 

	tz += 0;
	for (int i = 0; i < sizeof(g_vertex_buffer_data) / sizeof(g_vertex_buffer_data[0]); i += 3)
	{
		float x, y, z;
		if (g_vertex_buffer_data[i+0] < 0)
			x = tx ;
		else
			x = tx + cubeSize / Settings::scale.x *.75;

		if (g_vertex_buffer_data[i + 1] < 0)
			y = ty; //  -cubeSize / Settings::scale.y / 2;
		else
			y = ty + cubeSize / Settings::scale.y;

		if (g_vertex_buffer_data[i + 2] < 0)
			z = tz  ;
		else
			z = tz + cubeSize / Settings::scale.z;


		vertex_vec.push_back(glm::vec3(x, z, y));
		//yellow (type 2)
		float u = 0.25f;
		float v = 0.25f;
		
			 //red
		if (type == 1)
		{
			u = 0.75f;
			v = 0.75f;
		}
 
		if (type == 3)
		{
		 	v = 0.75f;
		}
	 

		uv_vec.push_back(glm::vec2(u, v));

	}


}



void Landscape::add3dMarker(float tx, float ty, float tz, int type, std::string text, float width, float height, float cubeXSize, float cubeYSize)
{
	//this version has specific sizes, so it is definitely not to be centered
	float cubeSize = std::min(cubeXSize, cubeYSize);
	tz += 0;
	for (int i = 0; i < sizeof(g_vertex_buffer_data) / sizeof(g_vertex_buffer_data[0]); i += 3)
	{
		float x, y, z;
		if (g_vertex_buffer_data[i + 0] < 0)
			x = tx;
		else
			x = tx + cubeXSize  ;

		if (g_vertex_buffer_data[i + 1] < 0)
			y = ty; //  -cubeSize / Settings::scale.y / 2;
		else
			y = ty + cubeYSize ;

		if (g_vertex_buffer_data[i + 2] < 0)
			z = tz;
		else
			z = tz + 50 / Settings::scale.z;


		vertex_vec.push_back(glm::vec3(x, z, y));
		//yellow (type 2)
		float u = 0.25f;
		float v = 0.25f;

		//red
		if (type == 1)
		{
			u = 0.75f;
			v = 0.75f;
		}

		if (type == 3)
		{
			v = 0.75f;
		}

		uv_vec.push_back(glm::vec2(u, v));

	}
}

void Landscape::appendLine(int ax)
{
	int steps = 250;



#if TOC_VERSION
	if (camera == NULL)
		return;
#else
	return;
#endif


	float size = viewData.worldSizeMz / steps;
	float sizey = viewData.worldSizeLc / steps;



	float yscale = Settings::correlateRatio;

	if (yscale < 0)
	{
		yscale = -1 / (yscale - 1);
	}
	else
		yscale = yscale + 1;


	auto camtarget = camera->currentTarget;
	auto pos2d = get2d(camtarget.mz, camtarget.lc, 0);


	double ranges[] = { .00001, .0001, 0.01, 0.1, 1, 10,100,1000,10000 };
	double range = 1000;
	int cnt = 0;
	int rangeIndex = 0;
	for (auto r : ranges)
	{
		auto pos2da = get2d(camtarget.mz + r, camtarget.lc, 0);
		auto dist = glm::distance(pos2d, pos2da);



		if (dist > 30)
		{
			range = r;
			rangeIndex = cnt;
			break;
		}
		cnt++;
	}

	{
		static int lastRange = 0;
		static int count = 0;
		count++;
		if (count & 31)
		{
			rangeIndex = lastRange;
			range = ranges[rangeIndex];


		}
		lastRange = rangeIndex;

	}



	float xoff = (worldMzRange.max - worldMzRange.min) * Settings::correlateOffset;


	// goes to *2 just because can start below 0, and it's easier

	double world_ratio = (worldMzRange.max - worldMzRange.min) / (worldLcRange.max - worldLcRange.min);
	yscale *= world_ratio;



	float yoff = (worldMzRange.min - worldLcRange.min);


	if (Settings::autoCorrelate)
		for (int i = -steps; i < steps * 2; i++)
		{
			float x1 = (float)worldMzRange.min + ((worldMzRange.max - worldMzRange.min) / steps) * i;
			float y1 = (float)((worldLcRange.max - worldLcRange.min) / steps) * i;

			float pos = std::min(x1, y1);

			float x = x1 + xoff;
			//	float y = (float)worldLcRange.min + ((worldLcRange.max - worldLcRange.min) / steps) * i * yscale;
			float y = y1 * yscale + worldLcRange.min + yoff;


			if ((x < worldMzRange.min) || x > worldMzRange.max)
				continue;

			if (y > worldLcRange.max)
				break;

			addDSquare(x, y, 0, size * 1, sizey * yscale);



		}



	if (Settings::addGridLines == false)
		return;


	for (int r = 0; r < 2; r++)
	{
		float x = (float)worldMzRange.min;
		steps = 30;
		size = viewData.worldSizeMz / 50000;
		sizey = viewData.worldSizeLc / steps;

		size = range;
		if (size > 400)
			size = 400;

		//	auto pos2d = get2d(camtarget.mz, camtarget.lc, 0);

		x = camtarget.mz - (range * 50);
		x = (int)(x / range);
		x *= range;


		cnt = 0;
		while (x < worldMzRange.max)
		{
			for (float i = 0; i < steps * 2; i++)
			{
				float y = (float)worldLcRange.min + ((worldLcRange.max - worldLcRange.min) / steps) * i;



				auto draw_size_y = sizey;
				if ((y - draw_size_y / 2) >= worldLcRange.max)
					continue;
				if ((y + draw_size_y / 2) >= worldLcRange.max)
				{

					//		draw_size_y = (worldLcRange.max - y)  ;
				}

				//		y = worldLcRange.max;
						//		 	draw_size_y = (worldLcRange.max - y)  *2;

				addSquare(x, y, 0, size * .5, draw_size_y, rangeIndex);



			}
			x += range;
			if (cnt++ > 100)
				break;
		}

		range *= 10;
		rangeIndex++;
	}

}






void Landscape::appendLiney(int ax)
{
	int steps = 250;



#if TOC_VERSION
	if (camera == NULL)
		return;
#else
	return;
#endif


	float sizex = viewData.worldSizeMz / steps;
	float size = viewData.worldSizeLc / steps;



	auto camtarget = camera->currentTarget;
	auto pos2d = get2d(camtarget.mz, camtarget.lc, 0);


	double ranges[] = { .00001, .0001, 0.01, 0.1, 1, 10,100,1000,10000 };
	double range = 1000;
	int cnt = 0;
	int rangeIndex = 0;
	for (auto r : ranges)
	{
		auto pos2da = get2d(camtarget.mz, camtarget.lc + r, 0);
		auto dist = glm::distance(pos2d, pos2da);



		if (dist > 30)
		{
			range = r;
			rangeIndex = cnt;
			break;
		}
		cnt++;
	}


	{
		static int lastRange = 0;
		static int count = 0;
		count++;
		if (count & 31)
		{
			rangeIndex = lastRange;
			range = ranges[rangeIndex];


		}
		lastRange = rangeIndex;

	}

 
 

	for (int r = 0; r < 2; r++)
	{
		float x = (float)worldMzRange.min;
		steps = 30;
 
	 
			sizex = viewData.worldSizeMz / steps;

		size = range;
		if (size > 200)
			size = 200;


		//	auto pos2d = get2d(camtarget.mz, camtarget.lc, 0);

		float y = camtarget.lc - (range * 50);
		y = (int)(y / range);
		y *= range;


		cnt = 0;
		while (y < worldLcRange.max)
		{
			for (float i = 0; i < steps * 2; i++)
			{
				float x = (float)worldMzRange.min + ((worldMzRange.max - worldMzRange.min) / steps) * i;


				auto draw_size_y = sizex;
				if ((x - draw_size_y / 2) >= worldMzRange.max)
					continue;

			addSquare(x, y, 0, sizex, size * .5, rangeIndex);



			}
			y += range;
			if (cnt++ > 100)
				break;
		}

		range *= 10;
		rangeIndex++;
	}

}







void Landscape::drawPinstripes()
{
	// does not need to be separated - maybe if there is a priority change
	return;
 
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

	
 	appendLine(0);
	if (Settings::preGridLines)
		appendLiney(1);

	if ((cubeMesh == NULL) && vertex_vec.size() > 0)
	{
 
	

		Mesh* newMesh = new Mesh(vertex_vec, uv_vec);
		cubeMesh = new GLMesh(newMesh, false);
		delete newMesh;
	}

	if (cubeMesh != NULL)
	{
 
		Render::drawCubeMesh(cubeMesh, 0);
	}
	

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

void Landscape::setInfo()
{
	addInfo("<b>File Information");

	addInfo(Settings::xLabel);
	addInfo("<c>range:");
	std::string mzr = std::to_string(worldMzRange.min) + " - " + std::to_string(worldMzRange.max);
	if (worldSignalRange.max < 0.1)
		mzr = "  ";

		addInfo(mzr);
		addInfo(Settings::yLabel +" range:");

		if (worldSignalRange.max < 0.1)
			addInfo("  ");
		else

		addInfo(std::to_string(worldLcRange.min) + " - " + std::to_string(worldLcRange.max));


		std::ostringstream streamObj;
		// Set Fixed -Point Notation
		streamObj << std::scientific << std::setprecision(2) << worldSignalRange.max;

		
 
	addInfo("Max. Intensity : " + streamObj.str());
	
	std::ostringstream streamObj2;
	streamObj2.imbue(std::locale(""));

	streamObj2   << numDataPoints;

	addInfo("Num. points : " + streamObj2.str());
	if (annotations.size() > 0)
	{
		std::ostringstream streamObj2;
		streamObj2.imbue(std::locale(""));

		streamObj2 << annotations.size();
		addInfo("Num. annotations : "   +streamObj2.str());
	}

	addInfo("");

}


void Landscape::addMarker(float tx, float ty, float tz, int i, std::string text, float width, float height, float cubeSize)
{
	
 

		glm::vec4  mid = transform({ tx,ty,tz });

		tz = 0;
		 


		if (cubeSize > 0)
		{
			add3dMarker(mid.x, mid.z, mid.y, i, text, width, height, cubeSize);
			add3dMarker(mid.x, mid.z, mid.y, 3, text, width, height, 10);
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

void Landscape::addMarker(float tx, float ty, float tz, int i, std::string text, float width, float height, float cubeSize, float cubeXSize, float cubeYSize)
{



	glm::vec4  mid = transform({ tx,ty,tz });

	tz = 0;



	if (cubeXSize > 0)
	{
		add3dMarker(mid.x, mid.z, mid.y, i, text, width, height, cubeSize);
		add3dMarker(mid.x, mid.z, mid.y, 3, text, width, height, cubeXSize, cubeYSize);
	}

	auto m = transformMatrix * mid;


	m = m / (m.w);

	if (m.z < 1)
		if (m.z > 0.0001)
			if (std::abs(m.x) < 1.1)
				if (std::abs(m.y) < 1.1)
				{
					int label_x = (int)(((m.x + 1) * viewport.x / 2) - (width / 2));


					int label_y = (int)(((-m.y + 1) * viewport.y / 2) - (height / 2));

					Label label = { label_x,label_y,text,0 };

					addLabel(i + 1, label);
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

	// z1 /= Settings::peakScale;


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

			// check children are all ready

//			if ((child->getScreenSize() >= -1)) // minSize)  )

			if ((child->getScreenSize() >  minSize)  )
			{
				visChildren++;


				if (canDraw(child))
					readyChildren++;

 
					
			}

			if (Globals::currentTime.time - child->lastDrawn.time < 3e6)
				readyChildren++;

		}

		bool drawChildren = false;
		if (visChildren > 0)
		{
			if ((Globals::currentTime.time - tile->childTime.time) < 1e6)
			{
			//	drawChildren = true;

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


			//drawCallback(tile, false);


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
		


	//	 	if (tile->getScreenSize() > 0.01)
		//if (tile->isOnScreen())
		if ((tile->drawStatus == DrawStatus::ready)
			|| (tile->drawStatus == DrawStatus::reCreatingMesh)
			|| (tile->drawStatus == DrawStatus::reCreateMesh)
			|| (tile->drawStatus == DrawStatus::reCreateGLMesh)
			|| (tile->drawStatus == DrawStatus::reCreatingGLMesh))
		{
		 
			// std::cout << " stgatus "<< (int) tile->drawStatus << "  " << tile->getScreenSize() <<  "   " << tile->id <<"  : " << tile->LOD <<"\n";
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
 
 // take them in order of when they were last actually drawn
	// in the event of a tie, remove lowest lod?
 
	

	if (loadedDataTiles.size() > tilesInRam)  
	{
 
		//copying a priority value to not invalidate sort
		for (auto t : loadedDataTiles)
			t->storePriority();

		std::sort(loadedDataTiles.begin(), loadedDataTiles.end(), Tile::compareTilePtrReverse);
		Tile* next = loadedDataTiles.back();
		std::cout << 1e-6*(Globals::currentTime.time - loadedDataTiles[0]->lastDrawn.time) << "  most recent \n";
		std::cout << loadedDataTiles.back()-> id << "   " <<  1e-6 * (Globals::currentTime.time - loadedDataTiles.back()->lastDrawn.time) << "  least recent \n";
		std::cout << loadedDataTiles.back()->id << "   " << 1e-6 * (Globals::currentTime.time - loadedDataTiles.back()->lastLoaded.time) << "  least recent \n";

		int numToClear = 90;

		//clear max 90 per frame
 		while (loadedDataTiles.size() > std::max(1, tilesInRam - numToClear))
		{

	 
		 

			Tile* next = loadedDataTiles.back();
			loadedDataTiles.pop_back();

		 // if it was recently loaded, don't remove it (and will assume that we need more memory)
			if ((Globals::currentTime.time - next->lastLoaded.time) < (30*1e6))
			{
				
				tilesInRam = tilesInRam * 1.4;
				std::cout << " Increase ram usage " << tilesInRam << "\n";

				numToClear++;

				notReady.push_back(next);
				continue;

			}

			if (next->unLoad() == false)
			{
				numToClear++;
				notReady.push_back(next);

			}

		}
		for (auto t : notReady)
			loadedDataTiles.push_back(t);

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
	
	//  can we draw this one?
	if (tile->getScreenSize() > min_draw_size)
		if ((tile->drawStatus == DrawStatus::ready)
			|| (tile->drawStatus == DrawStatus::reCreatingMesh)
			|| (tile->drawStatus == DrawStatus::reCreateMesh)
			|| (tile->drawStatus == DrawStatus::reCreateGLMesh)
			|| (tile->drawStatus == DrawStatus::reCreatingGLMesh))
		{
			return true;
		}

	//only check chilren if this is quite large
 	if ((tile->getScreenSize() > maxSize) )



	for (auto child : children)
	{
		 
		//if ((child->getScreenSize() >-2)) //    minSize)  )
			if ((child->getScreenSize() > minSize)  )
		{
			visChildren++;
			if (canDraw(child))
				readyChildren++;
		}

 
		// if any of the children were drawn recently, then we can draw them
		// (to prevent removing children, then re-adding them moments later)
		//may leave a blank space for a moment, but it's better than removing and re-adding
	 
		if (Globals::currentTime.time - child->lastDrawn.time < 2e6)
		{
			readyChildren += 256;
		}

	}

	//we can definitely draw children instead
	if ((visChildren > 0) && (visChildren == readyChildren))
		return true;


	
	return false;
}


bool stillDrawing = false;


#if 0
//non-parallel version

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



	if ((targetMesh == NULL))
	{

		//		
		static const GLfloat g_vertex_buffer_data[] = { 0,0,0,  0,0,1,  1,0,0 , 0,0,1,   1,0,0,  1,0,1 };

		std::vector<glm::vec3> vertex_vec;
		std::vector<glm::vec2> uv_vec;

		for (int i = 0; i < sizeof(g_vertex_buffer_data) / sizeof(g_vertex_buffer_data[0]); i += 3)
		{
			GLfloat x, y, z;
			x = (g_vertex_buffer_data[i + 0] - .5f) * -300;
			y = (g_vertex_buffer_data[i + 1]) * 300;
			z = (g_vertex_buffer_data[i + 2] - .5f) * 300;
			vertex_vec.push_back(glm::vec3(x, y, z));
			uv_vec.push_back(glm::vec2(-g_vertex_buffer_data[i + 0], -g_vertex_buffer_data[i + 2]));
		}


		Mesh* newMesh = new Mesh(vertex_vec, uv_vec);
		targetMesh = new GLMesh(newMesh, false);

		delete newMesh;
	}

	if (targetMesh != NULL)
		Render::drawCubeMeshDirection(targetMesh, 0);


}


#else

 void Landscape::drawTiles(const std::vector<Tile*> draw_tiles)
{
	 
 

	 Globals::setCurrentTime();

 


	if (stillDrawing)
 		 return;

 

	if (draw_tiles.size() < 1)
		return;
	if (drawCallback == NULL)
		return;
	
	stillDrawing = true;
 
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
 
		 
	}


	 
	 
	Render::copyBuffer();

	stillDrawing = false;
 

}






#endif




void Landscape::updateViewport(int x, int y)
{
	
	viewport.x = x;
	viewport.y = y;
}

void Landscape::addPendingTiles()
{
	if (new_tiles.size() < 1)
		return;
	//  
	//adding new tiles is to buffer to prevent delays due to thread safety

	tileLock.lock();

	for (Tile* newTile : new_tiles)
	{
		tiles.push_back(newTile);
	}
	new_tiles.clear();

	tileLock.unlock();
}


void Landscape::updateLandscape(glm::dmat4 matrix)
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

	//drawTiles(tiles);
	Globals::setCurrentTime();
#if 1

	if (tiles.size() < 1)
		return;
	if (drawCallback == NULL)
		return;



 
	
#ifdef THREADED 
	{
		std::vector<Tile*> copyTiles(tiles);
		static int cnt = 0;
		cnt++;
		


		//maybe don't need to do this every frame, it can take more than 1 frame to do
		int freq = 0;

		if (0)
		std::cout << cnt << "\n";
		

		if ((cnt &freq) == 0)
		{
			std::thread t1([this, copyTiles] {drawTiles(copyTiles); });
			t1.detach();

//			drawTiles(copyTiles);


		}
	}
#else
	drawTiles(tiles);
#endif
	/*
	if (1)
	{
		std::thread t1([this, copyTiles] {drawTiles(copyTiles); });
		t1.detach();
	}
	else
	 drawTiles(tiles);

	 */


 
	Render::drawDeferred();
	Render::drawTarget();
	manageQueue();

	drawPinstripes();


	Render::resetUV();

	drawCubes();

 

	static GLMesh* targetMesh = NULL;


	//this regenerates the meshes list every frame
	//no longer use target

 
	if ((targetMesh == NULL))
	{

		//		
		static const GLfloat g_vertex_buffer_data[] = { 0,0,0,  0,0,1,  1,0,0 , 0,0,1,   1,0,0,  1,0,1 };

		std::vector<glm::vec3> vertex_vec;
		std::vector<glm::vec2> uv_vec;

		for (int i = 0; i < sizeof(g_vertex_buffer_data) / sizeof(g_vertex_buffer_data[0]); i += 3)
		{
			GLfloat x, y, z;
			x = (g_vertex_buffer_data[i + 0] - .5f) * -300;
			y = (g_vertex_buffer_data[i + 1]) * 300;
			z = (g_vertex_buffer_data[i + 2] - .5f) * 300;



			vertex_vec.push_back(glm::vec3(x, y, z));




			uv_vec.push_back(glm::vec2(-g_vertex_buffer_data[i + 0], -g_vertex_buffer_data[i + 2]));
		}


		Mesh* newMesh = new Mesh(vertex_vec, uv_vec);
		targetMesh = new GLMesh(newMesh, false);

		delete newMesh;
	}

	if (targetMesh != NULL)
		Render::drawCubeMeshDirection(targetMesh, 0);

#endif

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
		if (mz >= tile->mzRange.min - .01)
			if (mz <= tile->mzRange.max + .01)
				if (lc >= tile->lcRange.min-.01)
					if (lc <= tile->lcRange.max + .01)
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

 
	// std::cout << best.mz << " , " << best.mz2 << "\n";

	// std::cout << dx << " , " << dy << ", " << dz << "\n";
 	DataPoint p = { best.mz,best.lc,best.signal };
	//p.mz2 = best.mz2;
	//p.lc2 = best.lc2;
	while (best.tile != NULL)
	{
		// std::cout << best.tile->LOD << "   " << best.tile->id << "   " << best.tile->getScreenSize() << "\n";
		best.tile = best.tile->parent;
	}
	
	

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


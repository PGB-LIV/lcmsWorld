#include "Tile.h"

#include <algorithm>
#include <deque>
#include "Globals.h"
#include "Error.h"
#include "Settings.h"
#include "Camera.h"

std::vector<Tile*> Tile::tileBuffer;

static int version_number = 123;
 
static int count_id = 0;
Tile::Tile(int lod, DataSource src, Landscape* o)
{
	owner = o;
	id = count_id++;

	LOD = lod;
	source = src;
 
	random = std::rand();

	
	owner->registerTile(this);

}

float Tile::visTime = 0.2 * 1e6;

float XS_MIN = -1;
float XS_MAX = 1;
float YS_MIN = -1;
float YS_MAX = 1;
//Line Clipping Algorithm
//Liang-Barsky

int clipTest(float p, float q, float& u1, float& u2)
{
float r;
if (p < 0)
{
	r = q / p;
	if (r > u2)
		return 0;
	else if (r > u1)
		u1 = r;
}
else if (p > 0)
{
	r = q / p;
	if (r < u1)
		return 0;
	else if (r < u2)
		u2 = r;
}
else if (q < 0)
	return 0;
return 1;
}

void clipping(float& x1, float& y1, float& x2, float& y2)
{
	const float dx = x2 - x1;
	const float dy = y2 - y1;
	float u1 = 0;
	float u2 = 1;
	if ((clipTest(-dx, x1 - XS_MIN, u1, u2)) &&
		(clipTest(dx, XS_MAX - x1, u1, u2)) &&
		(clipTest(-dy, y1 - YS_MIN, u1, u2)) &&
		(clipTest(dy, YS_MAX - y1, u1, u2)))
	{
		if (u2 < 1)
			x2 = x1 + (u2 * dx),
			y2 = y1 + (u2 * dy);
		if (u1 > 0)
			x1 += (u1 * dx),
			y1 += (u1 * dy);
	}
}

void Tile::setScreenSizeMaxSibling(std::vector<Tile*> sibs)
{
	auto maxSa = screenArea;
	auto maxCd = cameraDistance;
	for (auto child : sibs)
	{
		maxSa = std::max(child->screenArea, maxSa);
		maxCd = std::min(child->cameraDistance, maxCd);

	}
	screenArea = maxSa;
	cameraDistance = maxCd;
}

void Tile::setScreenSize(glm::mat4 matrix, glm::vec2 view)
{
	Camera* c = owner->getCamera();
	auto centre_mz = c->currentTarget.mz;
	auto centre_lc = c->currentTarget.lc;


	 

	if (owner->xScale == 0)
	{
		screenArea = 0;
		if (LOD == 0)
			screenArea = 1;

		
	 
		return;
	}
	double x1 = mzRange.min;
	double x2 = mzRange.max;
	double y1 = lcRange.min;
	double y2 = lcRange.max;
#if 0
	glm::vec4  p1 = transform({ x1,y1,0 });
	glm::vec4  p2 = transform({ x1,y2,0 });
	glm::vec4  p3 = transform({ x2,y1,0 });
	glm::vec4  p4 = transform({ x2,y2,0 });

	p1 = matrix * p1;
	p1 = p1 / (p1.w);
	p2 = matrix * p2;
	p2 = p2 / (p2.w);
	p3 = matrix * p3;
	p3 = p3 / (p3.w);
	p4 = matrix * p4;
	p4 = p4 / (p4.w);


	auto px0 = p1.x;
	auto px1 = p2.x;
	auto px2 = p3.x;;
	auto py0 = std::min(1.0f, p1.y);
	auto py1 = std::min(1.0f, p2.y);
	auto py2 = std::min(1.0f, p3.y);
	auto area = std::abs(px0 * (py1 - py2) + px1 * (py2 - py0) + px2 * (py0 - py1)) / 2;

	auto minx = std::abs(p1.x);
	minx = std::min(minx, std::abs(p2.x));
	minx = std::min(minx, std::abs(p3.x));
	minx = std::min(minx, std::abs(p4.x));

	auto miny = std::abs(p1.y);
	miny = std::min(miny, std::abs(p2.y));
	miny = std::min(miny, std::abs(p3.y));
	miny = std::min(miny, std::abs(p4.y));
	if (miny > 1.1)
		return;
	if (id == 0)
		std::cout << area << "  " << minx << "," << miny << "\n";

	auto maxy = std::max(p1.y, p2.y);
	maxy = std::max(maxy, p3.y);
	if (maxy > 1.2)
		return;

	screenArea = area / 100.0;
	onScreenNow();
	//	std::cout << area << "\n";
	return;

#else
	double z1 = 0;
	double z2 = maxSignal;
	
	// To reduce cpu load, don't do them all every frame (mask = 2^n-1)
	int mask = 3;

	if ((random & mask) != (Globals::loopCount & mask))
		return;
 
	//if it's  off-screen currently, eventually assume that it will not be a priority
	screenLocation += .001f;

	int xs = 15;
	int ys = 16;
	int zs = 3;
	if (LOD <2 )
	{
		xs = 12;
		ys = 11;
		zs = 3;
	}

	float min_z = 2.1f;
 
	float min_xy = 12.1f;
	glm::vec4 leftPoint = { -10,0,0,-1 };
	glm::vec4 rightPoint = { 10,0,0, -1 };
	float left = -3e27f;
	float right = 3e27f;

	double xp = centre_mz;
	xp = std::max(xp, x1);
	xp = std::min(xp, x2);

	double yp = centre_lc;
	yp = std::max(yp, y1);
	yp = std::min(yp, y2);
	if (1)
	{
		x1 = xp;
		x2 = xp;
		y1 = yp;
		y2 = yp;
		xs = 1;
		ys = 1;
	}

 

	for (int k = 0; k < zs; k++)
	{
 

		double zp = ((z2 - z1) * k / zs) + z1;
		for (int i = 0; i < xs; i++)
		{
 

			double xp = ((x2 - x1) * i / xs) + x1;
			for (int j = 0; j < ys; j++)
			{
				double yp = ((y2 - y1) * j / ys) + y1;



				glm::vec4  point = transform({ xp,yp,zp });
				auto tp = matrix * point;
				tp = tp / (tp.w);

				auto ydist = tp.y + 0.25f;

				auto xy = std::sqrt(tp.x*tp.x + ydist * ydist);
				min_xy = std::min(min_xy, xy);

				if (tp.z < 1)
					if (tp.z > 0.0001)
					{


#if 0
						int label_x = (int)((tp.x + 1)*view.x / 2);
						int label_y = (int)((-tp.y + 1)*view.y / 2);

						std::string label_text = std::to_string(xy);
						Label label = { label_x,label_y,label_text,0 };

						owner->addLabel(i, label);

#endif



						if (std::abs(tp.x) < 2.1)
							if (std::abs(tp.y) < 2.1)
							{
								min_z = std::min(tp.z, min_z);

							}

					}
				 

			}
		}
	}
	 

	if (min_z < 2)
	{
 
		float z = (min_z*2)-1;
		
		float nearP = Settings::nearPlane;
		float farP = Settings::farPlane;

		float linearDepth = (2.0f * nearP * farP) / (farP + nearP - z * (farP - nearP));
		linearDepth /= (farP - nearP);

		linearDepth = std::sqrt(linearDepth);
		linearDepth = std::sqrt(linearDepth);

		cameraDistance = std::max(1-(linearDepth*2.3f),0.0f);
	
	

		float mx = mzRange.max - mzRange.min;
		float ly = lcRange.max - lcRange.min;
		mx *= owner->xScale;
		ly *= owner->yScale;
		float area = (mx * ly) * Settings::DetailConstant;
		area = std::sqrt(area);

		 //a coarse approach to adjust by size of model
		area *= Settings::detail-0.9f;
		area *= (float) Globals::windowScale;
 

 		{
		 
	 
			if (owner->numDataPoints < 1.5e8)
				area *= 2;

			if (owner->numDataPoints < 8.5e8)

				area *= 2;
		}

		screenArea = (1.0f - min_z) * area;

 
	 	screenArea *=  .5f;
 
 

		if ((screenArea < lastScreenArea ) && (screenArea > (lastScreenArea*7/8)) )
			screenArea = lastScreenArea;
		
		lastScreenArea = screenArea;
		screenLocation = min_xy;



		onScreenNow();
		return;
	}
 

	if (min_xy < 2.5)
	{
		

		//enough to load, not enough to require display
		screenArea = 0.0041 * 2 * 4; //  0.004f * 2;;


	//	screenArea = -2;


		if ((screenArea < lastScreenArea) )
			screenArea = lastScreenArea;
		lastScreenArea = screenArea;

		screenLocation = min_xy;
		return;
	}

	screenArea = 0; //  screenArea * .9;
	return;

	//to draw labels on tiles
#if 0  
	float tx = (x1 ) ;
	float ty = (y1 ) ;
 

	tx = (x1 + x2) / 2;
	ty = (y1 + y2) / 2;

	if (LOD != 1)
	return;
 
 
	 
	Tile* tile = this;

 
	for (int i = 0; i < 2; i++)
	{
		// screenArea = 0;
		tx = x1;
		ty = y1;
		if (i == 0)
		{
			ty = (y1 + y2) / 2;
			tx = (x1 + x2) / 2;
		}

		glm::vec4  mid = transform({ tx,ty,0 });
		auto m = matrix * mid;
		m = m / (m.w);

		if (m.z < 1)
			if (m.z > 0.0001)
				if (std::abs(m.x) < 1.1)
					if (std::abs(m.y) < 1.1)
					{
						int label_x = (int)((m.x + 1)*view.x / 2);


						int label_y = (int)((-m.y + 1)*view.y / 2);
						std::string label_text = std::to_string(LOD) + " " + std::to_string(id) + " " + std::to_string(screenArea);
						if (parent != NULL)

							label_text = std::to_string(LOD) + " " + std::to_string(id) + " " + std::to_string(parent->id) + " " + std::to_string(screenArea);

						label_text = std::to_string((int)tx);
						if (i > 0)
							label_text = std::to_string((int)ty);
						Label label = { label_x,label_y,label_text,0 };

						if (screenArea > 0.0001)
							owner->addLabel(i, label);
					}
	}
#endif
#endif
}

inline glm::vec4  Tile::transform(glm::vec3 input)
{
	//todo
	//owner dereferencing should be removed, and values cached
 


	auto y1 = input.y;
	glm::vec4  result;
	y1 -= (owner->worldLcRange.min + owner->worldLcRange.max) / 2;


	y1 *= owner->yScale;

	auto x1 = input.x;
	x1 -= (owner->worldMzRange.min + owner->worldMzRange.max) / 2;
	x1 *= (mzFloat)owner->xScale;


	auto z1 = input.z;
	z1 *= owner->zScale;
	//then convert them - e.g., function could be logarithmic
	//note it is already normalised into 3d world space, which is current undefined
	z1 = Mesh::convertZ(z1);
	/*

	result.x = x1 * Settings::scale.x;
	result.z = y1 * Settings::scale.y;
	result.y = z1 * Settings::scale.z;
	*/
	result.x = x1;
	result.z = y1;
	result.y = z1;
//	std::cout << Settings::xScale << " , " << Settings::yScale << ", " << Settings::zScale << "\n";
	result.w = 1;
	return result;

}



void Tile::updateWorldRange(MZDataInfo info)
{
	owner->updateWorldRange(info);
}

 


void Tile::setChildren()
{
	for (auto tile : tileBuffer)
	{
		int numIds = tile->childIds.size();

		tile->children.resize(numIds);
		for (int i = 0; i < numIds; i++)
		{
			tile->children[i] = tileBuffer[tile->childIds[i]];
			tile->children[i]->parent = tile;
		}
	}
}

int Tile::deserialise(byte* buffer)
{
	int ptr = 0;
	int ver = version_number;
 
	memcpy(&ver, &buffer[ptr], sizeof(ver));
	ptr += sizeof(ver);


	memcpy( &id, &buffer[ptr], sizeof(id));
	ptr += sizeof(id);

	if (tileBuffer.size() < id + 1)
		tileBuffer.resize(id + 1);
	tileBuffer[id] = this;

	memcpy( &LOD, &buffer[ptr], sizeof(LOD));
	ptr += sizeof(LOD);
	if (ver != version_number)
	{
		
		return -1;

	}
		memcpy(&type, &buffer[ptr], sizeof(type));
		ptr += sizeof(type);
 

	memcpy( &source, &buffer[ptr], sizeof(source));
	ptr += sizeof(source);

	memcpy( &mzRange, &buffer[ptr], sizeof(mzRange));
	ptr += sizeof(mzRange);

	memcpy( &lcRange, &buffer[ptr], sizeof(lcRange));
	ptr += sizeof(lcRange);


	memcpy(&maxSignal, &buffer[ptr], sizeof(maxSignal));
	ptr += sizeof(maxSignal);

 
	memcpy(&numDataPoints, &buffer[ptr], sizeof(numDataPoints));
	ptr += sizeof(numDataPoints);
 

	int numChildren;

	memcpy( &numChildren, &buffer[ptr], sizeof(numChildren));
	ptr += sizeof(numChildren);


	childIds.resize(numChildren);
	for (int i=0; i < numChildren; i++)
	{
		memcpy( &childIds[i], &buffer[ptr], sizeof(id));
		ptr += sizeof(id);
	}
	if (numChildren == 0)
	owner->numDataPoints += numDataPoints;

	return ptr;
}
const std::vector<byte>  Tile::serialise()
{
	std::vector<byte> buffer;
	
	return buffer;

}

//serialises directly to (allocated) memory
//returns size of data written
int  Tile::serialise(byte *buffer)
{
	
	int ptr = 0;

	int ver = version_number;
	
	memcpy(&buffer[ptr], &ver, sizeof(ver));
	ptr += sizeof(ver);


	memcpy(&buffer[ptr], &id, sizeof(id));
	ptr += sizeof(id);

	memcpy(&buffer[ptr], &LOD, sizeof(LOD));
	ptr += sizeof(LOD);

	memcpy(&buffer[ptr], &type, sizeof(type));
	ptr += sizeof(type);

	memcpy(&buffer[ptr], &source, sizeof(source));
	ptr += sizeof(source);

	memcpy(&buffer[ptr], &mzRange, sizeof(mzRange));
	ptr += sizeof(mzRange);

	memcpy(&buffer[ptr], &lcRange, sizeof(lcRange));
	ptr += sizeof(lcRange);

	memcpy(&buffer[ptr], &maxSignal, sizeof(maxSignal));
	ptr += sizeof(maxSignal);

 
	memcpy(&buffer[ptr], &numDataPoints, sizeof(numDataPoints));
	ptr += sizeof(numDataPoints);
	

	int numChildren = children.size();

	memcpy(&buffer[ptr], &numChildren, sizeof(numChildren));
	ptr += sizeof(numChildren);
 
	for (auto child : children)
	{
		int id = child->id;
		memcpy(&buffer[ptr], &id, sizeof(id));
		ptr += sizeof(id);
	}

	if(children.size() ==0)
	owner->numDataPoints += numDataPoints;
 	return ptr;

}

void Tile::setMZData(MZData* n) 
{ 
	hasMzData = false;
	mzdata = n; 
	type = n->type;
 
	n->setRange(true);
 

	mzRange = n->info.mzRange;
	lcRange = n->info.lcRange; 
	if (drawStatus == DrawStatus::noGLMesh)
	{
		std::cout << " clear error \n";
	}


	drawStatus = DrawStatus::noMesh;
	numDataPoints = n->info.num_points;
	maxSignal = n->info.signalRange.max;
 

	updateWorldRange(n->info);
	hasMzData = true;
}

/*
DrawStatus Tile::getChildrenStatus()
{
	if (children.size() == 0)
		return DrawStatus::noData;

	DrawStatus status = DrawStatus::ready;

	for (auto child : children)
	{
		if (child->drawStatus < status)
			status = child->drawStatus;
	}

	return status;
}
*/

std::mutex loadLock;
bool Tile::unLoad()
{
//	if (drawStatus != DrawStatus::ready)
//		return false;

	if (drawStatus == DrawStatus::ready)
	{
		clearGLMesh();
 
	}
	if (drawStatus == DrawStatus::noGLMesh)
	{
		clearMesh();
		drawStatus = DrawStatus::noMesh;

	}

	if (drawStatus == DrawStatus::noMesh)
	{
		loadLock.lock();
		if (drawStatus == DrawStatus::noMesh)
		{
					hasMzData = false;
					drawStatus = DrawStatus::noData;
					//need to make sure it's not loading!
 					delete mzdata;
					mzdata = NULL;
					
		}
		loadLock.unlock();
	}
	screenLocation = 20;
	screenArea = -2;
	lastScreenArea = -2;
	return true;

}
int numTileMeshesSet = 0;

void Tile::setMesh(Mesh* m) 
{
	lastLoaded = Globals::getCurrentTime();
	//testing - move it to the front of the queue
	//i.e. - preloaded means it should be considered drawn, to keep the two queues in balance
	lastDrawn = Globals::getCurrentTime();
	// mesh is currently deleted when GLMesh is created
	//so no need to delete old one here
	 
	numTileMeshesSet++;
	if (mesh != m)
	{
		//I will usually have deleted it anyway
		if (mesh != NULL)
			delete mesh;   
	}
	mesh = m;

	drawStatus = DrawStatus::noGLMesh;
 
}
void Tile::clearGLMesh(bool rebuild)
{
	if (drawStatus != DrawStatus::ready)
		return;
 

	drawStatus = DrawStatus::noGLMesh;
	
	delete wglMesh;
	delete glMesh;
 
	
	if (mesh == NULL)
	{
		drawStatus = DrawStatus::noMesh;

	}

}
void Tile::clearMesh(bool rebuild)
{
 

	if (rebuild)
	{
		if (drawStatus != DrawStatus::ready)
			return;
		drawStatus = DrawStatus::reCreateMesh;

	}
	else
	{
		if (drawStatus != DrawStatus::noGLMesh)
			return;
		drawStatus = DrawStatus::noMesh;
		if (mesh != NULL)
			delete mesh;
	}
	//
//	if (mesh != NULL)
//		delete mesh;
	mesh = NULL;
}

void Tile::clearMZData()
{
	if (drawStatus == DrawStatus::noGLMesh)
	{
		return;
	}
	drawStatus = DrawStatus::noData;

	if (mzdata != NULL)
	{
		hasMzData = false;
		delete mzdata;
		mzdata = NULL;
	}

	 
}

static std::mutex s_lock;
const std::vector<byte> Tile::serialiseData(int thread_id)
{
	

	s_lock.lock();
	if (mzdata == NULL)
		std::cout << " NULL mz\n";

	assert(mzdata != NULL);
	auto ret = mzdata->serialise(thread_id, true);
	 

	s_lock.unlock();
	return ret;
}

//This must only be called in the same thread as the GL drawing
// (hence it is a separate function from getGLMesh)

int numTileGLMeshesSet = 0;
int numTileMeshesDeleted = 0;
GLMesh* Tile::setGLMesh( bool recreate)
{
	if (mesh == NULL)
	{
		return NULL;
	}
	drawStatus = DrawStatus::creatingGLMesh;

	numTileGLMeshesSet++;
	 
	{
		auto standard = new GLMesh(mesh, true);
		auto wire =  new GLMesh(mesh, standard);
 
		if (recreate)
		{
			if (wglMesh != NULL)
				delete wglMesh;
			if (glMesh != NULL)
				delete glMesh;
		}
		wglMesh = wire;
		glMesh = standard;

	}
		drawStatus = DrawStatus::ready;

	delete mesh;

//	numTileMeshesDeleted++;
//	std::cout << numTileMeshesSet << " , " << numTileMeshesDeleted << "  " << numTileMeshesSet - numTileMeshesDeleted << "\n";
	//don't try and delete it again
	mesh = NULL;
	
	owner->dataLoaded(this);


	return glMesh;
}


std::mutex bufferLock;

void Tile::deSerialiseData(const std::vector<byte> &buf)
{

	bufferLock.lock();
	//lock / unlock is probably no longer necessary
	//but if so, won't cause any problems
 
	if (drawStatus == DrawStatus::noMesh)
	{
		bufferLock.unlock();
		return;
	}



	if (mzdata == NULL)
		mzdata = new MZData();
 
	mzdata->deSerialise(buf);

	assert(mesh == NULL);
	drawStatus = DrawStatus::noMesh;
	bufferLock.unlock();
	hasMzData = true;
}

Tile::~Tile()
{
	 
	//	if (mesh != NULL)
	//		delete mesh;
		if (mzdata != NULL)
			delete mzdata;

	 
	 
}

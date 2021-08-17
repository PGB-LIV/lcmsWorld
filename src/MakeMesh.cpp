
#include "MZData.h"
#include "Landscape.h"
#include "MZScan.h"
#include "Tile.h"
#include <iostream>  
#include <algorithm>
#include <set>
#include <sstream>
#include <mutex>
#include "Render.h"
#include <atomic>



const float noiseThreshold = 1; //set to +ve  to remove quads with little or no signal
							//if it's -ve, all will be included
static std::mutex scale_lock;
static bool mapSet = false;
const float uv_u = 0.25;
const float uv_0 = 0.25;
const float uv_1 = 0.25;

//for a proxmity map of annotations
//could potentially store distance?


//MAKE sure mapX is multiple of 8 (for bit-packing)
//uses 4mb of memory, which is reasonable
const int mapX = 1024 * 8;
const int mapY = 4096;
static byte map[mapX / 8][mapY];

void Landscape::setScale(Tile* tile)
{



	xScale = (float)(viewData.worldSizeMz / (worldMzRange.max - worldMzRange.min));

	std::cout << "set xscale " << xScale << " - " << worldMzRange.max << "\n";
	yScale = (float)(viewData.worldSizeLc / (worldLcRange.max - worldLcRange.min));
	zScale = (float)(viewData.peakHeight / worldSignalRange.max);

	Mesh::logzscale = (float)(viewData.peakHeight / std::sqrt(viewData.peakHeight));



	// std::cout << viewData.worldSizeLc << " lc size \n ";
	scalingValuesSet = true;


}
Mesh* Landscape::getJaggedMesh(Tile* tile)
{

	Range<mzFloat> mzRange = tile->getWorldMzRange();
	Range<lcFloat> lcRange = tile->getWorldLcRange();
	Range<signalFloat> signalRange = tile->getWorldSignalRange();

	auto mzData = tile->getMZData();
	auto scans = mzData->getScans();

	MZScan* first = scans[0];
	int width = (int)first->getMz().size();
	int height = (int)scans.size();

	int num_vertices = width * height * 6;
	int num_points = width * height * 6 * 2;

	std::vector<glm::vec3> vertex_vec;
	vertex_vec.reserve(num_vertices);

	std::vector<glm::vec2> uv_vec;
	uv_vec.reserve(num_vertices);

	std::vector<unsigned short> vb_vec;
	vb_vec.reserve(num_points);

	std::vector<float> attr_vec;
	attr_vec.reserve(num_vertices);


	//	float maxX = 0;
	float minX = 0;
	float maxY = 0;
	float minY = 0;
	int bIndex = 0;

	double sum = 0;
	for (int i = 0; i < height; i++)
	{
		MZScan *scan = scans[i];
		auto data = scan->getIntensity();
		for (auto x : data)
			sum += x;
	}



	for (int i = 1; i < height; i++)
	{
		MZScan *top = scans[i - 1];
		MZScan *bottom = scans[i];

		auto y1 = top->getLcTime();
		auto y2 = bottom->getLcTime();

		auto topMz = top->getMz();
		auto botMz = top->getMz();


		auto topI = top->getIntensity();
		auto botI = bottom->getIntensity();
		int yp = (int)((y1 - lcRange.min) * mapY / (lcRange.max - lcRange.min));

		y1 -= (lcRange.min + lcRange.max) / 2;
		y2 -= (lcRange.min + lcRange.max) / 2;

		y1 *= yScale;
		y2 *= yScale;

		width = topMz.size();
		if (width != top->getSize())
		{
			std::cout << width - top->getSize() << "  ";
		}

		for (int j = 1; j < width; j++)
		{


			auto x1 = topMz[j - 1];
			auto x2 = topMz[j];

			auto z1 = topI[j - 1];
			auto z3 = topI[j - 1];

			auto z2 = topI[j];
			auto z4 = z2;

			//an attempt to align, doesn't work
#if 0
 			
			int j2 = 1;

			auto z4b = z4;
			
			while (botMz[j2] < x1 && j2 < botI.size())
			{
			
			z4b = botI[j2];
			
			j2++;
			}
			
			auto diff = botMz[j2] - x1 ;
			if (abs(diff) < .01)
			{
				if ((z4b > z4*.5) && (z4b < z4*1.5))
				z4 = z4b;

			}
			 
#endif
			byte mapVal = 0;
			if (mapSet)
			{
				int xp = (int)((x1 - mzRange.min) * mapX / (mzRange.max - mzRange.min));
				mapVal = map[xp / 8][yp];
				mapVal &= (1 << (xp & 7));

			}



			insertData(tile, vertex_vec, uv_vec, vb_vec, attr_vec, x1, x2, y1, y2, z1, z2, z3, z4, mapVal, 2);


		}

	}




	Mesh* mesh;
	if (attr_vec.size() > 0)
		mesh = new Mesh(vertex_vec, uv_vec, vb_vec, attr_vec, true);
	else
		mesh = new Mesh(vertex_vec, uv_vec, vb_vec, true);

	return mesh;
}

//this version takes previous three points as points to include


bool inline Landscape::insertData(Tile* tile, std::vector<glm::vec3> &vertex_vec, std::vector<glm::vec2> &uv_vec, std::vector<unsigned short> &vb_vec, std::vector<float> &attr_vec, mzFloat x2, lcFloat y2, signalFloat z4, unsigned short p1, unsigned short p3, unsigned short p2, byte mapVal)
{


	Range<mzFloat> mzRange = tile->getWorldMzRange();
	Range<lcFloat> lcRange = tile->getWorldLcRange();
	Range<signalFloat> signalRange = tile->getWorldSignalRange();



	z4 *= zScale;

	//then convert them - e.g., function could be logarithmic
	//note it is already normalised into 3d world space, which is current undefined

	z4 = Mesh::convertZ(z4);





	x2 -= (mzRange.min + mzRange.max) / 2;


	x2 *= (mzFloat)xScale;
	//these are temporary 


	float yoff = 0;
	float zoff = 0;
	//points in Z shape




	glm::vec3 p4 = glm::vec3(x2, z4 + zoff, y2 + yoff);

	//push the triangles onto the vertices

	unsigned short startVertex = vertex_vec.size() - 1;



	vertex_vec.push_back(p4);


	vb_vec.push_back(p1);
	vb_vec.push_back(startVertex + 1);
	vb_vec.push_back(p3);
	vb_vec.push_back(p1);
	vb_vec.push_back(p2);
	vb_vec.push_back(startVertex + 1);

	z4 *= peakUV / viewData.peakHeight;




	z4 += zOffset;


	//uvs


	glm::vec2 c4 = glm::vec2(uv_u, z4);



	uv_vec.push_back(c4);

	float val = 1;
	if ((mapVal))
		val = 2;
	attr_vec.push_back(val);
	return true;
}


//this version takes previous two points as points to include
// was p2, p4, become p1,. p3

bool inline Landscape::insertData(Tile* tile, std::vector<glm::vec3> &vertex_vec, std::vector<glm::vec2> &uv_vec, std::vector<unsigned short> &vb_vec, std::vector<float> &attr_vec, mzFloat x2, lcFloat y1, lcFloat y2, signalFloat z2, signalFloat z4, unsigned short p1, unsigned short p3, byte mapVal)
{


	Range<mzFloat> mzRange = tile->getWorldMzRange();
	Range<lcFloat> lcRange = tile->getWorldLcRange();
	Range<signalFloat> signalRange = tile->getWorldSignalRange();




	//normalise the heights first
	z2 *= zScale;
	z4 *= zScale;

	//then convert them - e.g., function could be logarithmic
	//note it is already normalised into 3d world space, which is current undefined
	z2 = Mesh::convertZ(z2);
	z4 = Mesh::convertZ(z4);





	x2 -= (mzRange.min + mzRange.max) / 2;


	x2 *= (mzFloat)xScale;
	//these are temporary 


	float yoff = 0;
	float zoff = 0;
	//points in Z shape




	glm::vec3 p2 = glm::vec3(x2, z2 + zoff, y1 + yoff);

	glm::vec3 p4 = glm::vec3(x2, z4 + zoff, y2 + yoff);

	//push the triangles onto the vertices

	unsigned short startVertex = vertex_vec.size() - 1;


	//todo
	float val = 1;
	if ((mapVal))
		val = 2;

	attr_vec.push_back(val);
	attr_vec.push_back(val);

	vertex_vec.push_back(p2);
	vertex_vec.push_back(p4);

	vb_vec.push_back(p1);
	vb_vec.push_back(startVertex + 2);
	vb_vec.push_back(p3);
	vb_vec.push_back(p1);
	vb_vec.push_back(startVertex + 1);
	vb_vec.push_back(startVertex + 2);


	z2 *= peakUV / viewData.peakHeight;
	z4 *= peakUV / viewData.peakHeight;



	z2 += zOffset;

	z4 += zOffset;


	//uvs

	glm::vec2 c2 = glm::vec2(uv_u, z2);
	glm::vec2 c4 = glm::vec2(uv_u, z4);


	uv_vec.push_back(c2);
	uv_vec.push_back(c4);
	return true;
}

//standard version (no vb)
bool inline Landscape::insertData(Tile* tile, std::vector<glm::vec3> &vertex_vec, std::vector<glm::vec2> &uv_vec, mzFloat x1, mzFloat x2, lcFloat y1, lcFloat y2, signalFloat z1, signalFloat z2, signalFloat z3, signalFloat z4)
{


	if (z1 + z2 + z3 + z4 <= noiseThreshold)
		return false;



	Range<mzFloat> mzRange = tile->getWorldMzRange();
	Range<lcFloat> lcRange = tile->getWorldLcRange();
	Range<signalFloat> signalRange = tile->getWorldSignalRange();




	//normalise the heights first
	z1 *= zScale;
	z2 *= zScale;
	z3 *= zScale;
	z4 *= zScale;

	//then convert them - e.g., function could be logarithmic
	//note it is already normalised into 3d world space, which is current undefined
	z1 = Mesh::convertZ(z1);
	z2 = Mesh::convertZ(z2);
	z3 = Mesh::convertZ(z3);
	z4 = Mesh::convertZ(z4);





	x1 -= (mzRange.min + mzRange.max) / 2;
	x2 -= (mzRange.min + mzRange.max) / 2;


	x1 *= (mzFloat)xScale;
	x2 *= (mzFloat)xScale;
	//these are temporary 


	float yoff = 0;
	float zoff = 0;
	//points in Z shape



	glm::vec3 p1 = glm::vec3(x1, z1 + zoff, y1 + yoff);
	glm::vec3 p2 = glm::vec3(x2, z2 + zoff, y1 + yoff);
	glm::vec3 p3 = glm::vec3(x1, z3 + zoff, y2 + yoff);
	glm::vec3 p4 = glm::vec3(x2, z4 + zoff, y2 + yoff);

	//push the triangles onto the vertices

	//3 1 2, 4 3 2

	vertex_vec.push_back(p1);
	vertex_vec.push_back(p4);
	vertex_vec.push_back(p3);

	vertex_vec.push_back(p1); //4 3 2
	vertex_vec.push_back(p2);
	vertex_vec.push_back(p4);



	z1 *= peakUV / viewData.peakHeight;
	z2 *= peakUV / viewData.peakHeight;
	z3 *= peakUV / viewData.peakHeight;
	z4 *= peakUV / viewData.peakHeight;


	z1 += zOffset;
	z2 += zOffset;
	z3 += zOffset;
	z4 += zOffset;


	//uvs
	glm::vec2 c1 = glm::vec2(uv_0, z1);
	glm::vec2 c2 = glm::vec2(uv_0, z2);
	glm::vec2 c3 = glm::vec2(uv_1, z3);
	glm::vec2 c4 = glm::vec2(uv_u, z4);




	uv_vec.push_back(c1);
	uv_vec.push_back(c4);
	uv_vec.push_back(c3);

	uv_vec.push_back(c1);
	uv_vec.push_back(c2);
	uv_vec.push_back(c4);
	return true;
}

//with vb (no shared points)


bool inline Landscape::insertData(Tile* tile, std::vector<glm::vec3> &vertex_vec, std::vector<glm::vec2> &uv_vec, std::vector<unsigned short> &vb_vec, std::vector<float> &attr_vec, mzFloat x1, mzFloat x2, lcFloat y1, lcFloat y2, signalFloat z1, signalFloat z2, signalFloat z3, signalFloat z4, byte mapVal, int alwaysDraw)
{

	if (alwaysDraw == 0)
		if (z1 + z2 + z3 + z4 <= noiseThreshold)
			return false;

	bool noise = (z1 + z2 + z3 + z4 <= noiseThreshold);



	Range<mzFloat> mzRange = tile->getWorldMzRange();



	//normalise the heights first
	z1 *= zScale;
	z2 *= zScale;
	z3 *= zScale;
	z4 *= zScale;

	//then convert them - e.g., function could be logarithmic
	//note it is already normalised into 3d world space, which is current undefined
	z1 = Mesh::convertZ(z1);
	z2 = Mesh::convertZ(z2);
	z3 = Mesh::convertZ(z3);
	z4 = Mesh::convertZ(z4);



 
 

	x1 -= (mzRange.min + mzRange.max) / 2;
	x2 -= (mzRange.min + mzRange.max) / 2;


	x1 *= (mzFloat)xScale;
	x2 *= (mzFloat)xScale;



	float yoff = 0;
	float zoff = 0;
	//points in Z shape



	glm::vec3 p1 = glm::vec3(x1, z1 + zoff, y1 + yoff);
	glm::vec3 p2 = glm::vec3(x2, z2 + zoff, y1 + yoff);
	glm::vec3 p3 = glm::vec3(x1, z3 + zoff, y2 + yoff);
	glm::vec3 p4 = glm::vec3(x2, z4 + zoff, y2 + yoff);

	glm::vec3 p5 = glm::vec3(x1, 0, y1 + yoff);
	glm::vec3 p6 = glm::vec3(x2, 0, y1 + yoff);
	glm::vec3 p7 = glm::vec3(x1, 0, y2 + yoff);
	glm::vec3 p8 = glm::vec3(x2, 0, y2 + yoff);

	//push the triangles onto the vertices

	unsigned short startVertex = vertex_vec.size() - 1;

	vb_vec.push_back(startVertex + 1);
	vb_vec.push_back(startVertex + 4);
	vb_vec.push_back(startVertex + 3);
	vb_vec.push_back(startVertex + 1);
	vb_vec.push_back(startVertex + 2);
	vb_vec.push_back(startVertex + 4);





	vertex_vec.push_back(p1);
	vertex_vec.push_back(p2);
	vertex_vec.push_back(p3);
	vertex_vec.push_back(p4);

	z1 *= peakUV / viewData.peakHeight;
	z2 *= peakUV / viewData.peakHeight;
	z3 *= peakUV / viewData.peakHeight;
	z4 *= peakUV / viewData.peakHeight;


	z1 += zOffset;
	z2 += zOffset;
	z3 += zOffset;
	z4 += zOffset;


	//uvs
	glm::vec2 c1 = glm::vec2(uv_0, z1);
	glm::vec2 c2 = glm::vec2(uv_0, z2);
	glm::vec2 c3 = glm::vec2(uv_1, z3);
	glm::vec2 c4 = glm::vec2(uv_1, z4);

	uv_vec.push_back(c1);
	uv_vec.push_back(c2);
	uv_vec.push_back(c3);
	uv_vec.push_back(c4);

	//std::vector<byte> &attr_vec,
	float val = 1;
	if ((mapVal))
		val = 2;
	attr_vec.push_back(val);
	attr_vec.push_back(val);
	attr_vec.push_back(val);
	attr_vec.push_back(val);


	if (Settings::displaySides)
		if (alwaysDraw > 1)
			if (noise == false)
			{
				//	glm::vec3 p3 = glm::vec3(x1, z3 + zoff, y2 + yoff);
				//	glm::vec3 p4 = glm::vec3(x2, z4 + zoff, y2 + yoff);

				vertex_vec.push_back(p5);
				vertex_vec.push_back(p6);

				vertex_vec.push_back(p7);
				vertex_vec.push_back(p8);

				uv_vec.push_back(glm::vec2(uv_0, 0));
				uv_vec.push_back(glm::vec2(uv_0, 0));
				uv_vec.push_back(glm::vec2(uv_0, 0));
				uv_vec.push_back(glm::vec2(uv_0, 0));


				attr_vec.push_back(val);
				attr_vec.push_back(val);
				attr_vec.push_back(val);
				attr_vec.push_back(val);


				//rh edge
#if 1
				vb_vec.push_back(startVertex + 6);
				vb_vec.push_back(startVertex + 1);
				vb_vec.push_back(startVertex + 5);
				vb_vec.push_back(startVertex + 6);
				vb_vec.push_back(startVertex + 2);
				vb_vec.push_back(startVertex + 1);
#endif
				//lh edge

				vb_vec.push_back(startVertex + 8);
				vb_vec.push_back(startVertex + 7);
				vb_vec.push_back(startVertex + 3);
				vb_vec.push_back(startVertex + 8);
				vb_vec.push_back(startVertex + 3);
				vb_vec.push_back(startVertex + 4);


			}

	return true;
}

//Does not use vertex buffer - now outdated, but kept in codebase for testing alternative

Mesh* Landscape::makeMesh(Tile* tile)
{

	// caching mesh value?
	/*
	if (mesh != NULL)
		return mesh;
		*/
	int maxSkip = 0;
	int numSkip = 0;
	int total = 0;
	int inserted = 0;

	auto mzData = tile->getMZData();
	auto &scans = mzData->getScans();

	// can't make one without data
	if (scans.size() == 0)
		return NULL;

	auto type = mzData->type;

	//make a nre mesh
	if (type == MZDataType::jagged)
	{
		auto mesh = getJaggedMesh(tile);
		return mesh;
	}
	scale_lock.lock();
	if (scalingValuesSet == false)
	{
		setScale(tile);
	}
	scale_lock.unlock();





	Range<mzFloat> mzRange = tile->getWorldMzRange();
	Range<lcFloat> lcRange = tile->getWorldLcRange();
	Range<signalFloat> signalRange = tile->getWorldSignalRange();


	MZScan* first = scans[0];
	int width = (int)first->getMz().size();
	int height = (int)scans.size();

	int num_vertices = width * height * 6;

	std::vector<glm::vec3> vertex_vec;
	vertex_vec.reserve(num_vertices);

	std::vector<glm::vec2> uv_vec;
	uv_vec.reserve(num_vertices);




	//	float maxX = 0;
	float minX = 0;
	float maxY = 0;
	float minY = 0;
	int bIndex = 0;

	double sum = 0;
	for (int i = 0; i < height; i++)
	{
		MZScan *scan = scans[i];
		// std::cout << "scan width = " << scan->getIntensity().size() << "  " << scan->getLcTime() << " \n";
		auto data = scan->getIntensity();
		for (auto x : data)
			sum += x;
	}

	mzFloat maxX = 0;

	MZScan * blank = new MZScan(scans[height - 1]);


	blank->blank();


	for (int i = 1; i < height + 1; i++)
	{
		MZScan *top = scans[i - 1];
		MZScan *bottom = blank;
		auto y1 = top->getLcTime();
		auto y2 = bottom->getLcTime();
		//only overlap (to 0) the low-detail tiles
		if (i >= height)
		{

			if (tile->LOD > 3)
			{
				y2 = y1 + .000001f;
			}
		}
		else
			bottom = scans[i];





		auto topMz = top->getMz();
		auto botMz = bottom->getMz();

		if (type == MZDataType::square)
		{
			MZScan *zero = scans[0];

			topMz = zero->getMz();
			botMz = topMz;


		}

		auto topI = top->getIntensity();
		auto botI = bottom->getIntensity();

		y1 -= (lcRange.min + lcRange.max) / 2;
		y2 -= (lcRange.min + lcRange.max) / 2;

		y1 *= yScale;
		y2 *= yScale;


		int last = 0;
		for (int j = 1; j < width; j++)
		{

			if (topI.size() <= j)
				break;
			if (botI.size() <= j)
				break;

			auto x1 = topMz[j - 1];


			maxX = std::max(x1, maxX);
			if (x1 <= 0)
			{
				std::cout << "-ve x \n";
				//				continue;
			}
			last = j;



			auto x2 = topMz[j];

			auto z1 = topI[j - 1];
			auto z3 = botI[j - 1];

			auto z2 = topI[j];
			auto z4 = botI[j];

			//	z3 = 0;
			//	z1 = 322221;
			//	z2 = 322221;
			//	z4 = 0;



			total++;
			while ((j < width - 1) && (z1 == z2) && (z2 == topI[j + 1]) && (z3 == z4) && (z4 == topI[j + 1]))
			{
				numSkip++;

				total++;

				j++;

				x2 = topMz[j];
				z2 = topI[j];
				z4 = botI[j];

 

			}


			if (insertData(tile, vertex_vec, uv_vec, x1, x2, y1, y2, z1, z2, z3, z4))
			{
				bIndex += 6;
				inserted++;

			}




		}


		if (false)
		{
			//fill in end of empty row
			float x1 = topMz[last];


			while (x1 < mzRange.max)
			{


				float z1 = 0;
				float z2 = 0;
				float z3 = 0;
				float z4 = 0;

				float x2 = x1 + 1;

				if (insertData(tile, vertex_vec, uv_vec, x1, x2, y1, y2, z1, z2, z3, z4))
				{
					bIndex += 6;

				}

				x1 = x2;


			}

		}

	}

	delete blank;
	auto mesh = new Mesh(vertex_vec, uv_vec);

	// std::cout << total << "  " << numSkip << "  " << inserted << "  " << (total-numSkip-inserted) << "\n";
//	 std::cout << "#Num tris " << num_vertices << "  " << bIndex << " " << vertex_vec.size() << " sum = " << info.num_points << "\n";


	return mesh;
}


#define max_line_size 32768
unsigned short last_pos1y[max_line_size];
unsigned short last_pos2y[max_line_size];

//This is now used in preference to makemesh
Mesh* Landscape::makeMesh2(Tile* tile)
{


	int maxSkip = 0;
	int numSkip = 0;
	int total = 0;
	int inserted = 0;

	auto mzData = tile->getMZData();
	auto type = mzData->type;

	auto scans = mzData->getScans();
	// can't make one without data
	if (scans.size() == 0)
		return NULL;


	//make a nre mesh
	if (type == MZDataType::jagged)
	{
		auto mesh = getJaggedMesh(tile);
		return mesh;
	}
	scale_lock.lock();
	if (scalingValuesSet == false)
	{
		setScale(tile);
	}
	scale_lock.unlock();

	auto mzRange = worldMzRange;
	auto lcRange = worldLcRange;
	auto signalRange = worldSignalRange;


	MZScan* first = scans[0];
	int width = (int)first->getMz().size();
	int height = (int)scans.size();

	int num_vertices = width * height * 4;
	int num_points = width * height * 6;

	std::vector<glm::vec3> vertex_vec;
	vertex_vec.reserve(num_vertices);

	std::vector<glm::vec2> uv_vec;
	uv_vec.reserve(num_vertices);


	std::vector<unsigned short> vb_vec;
	vb_vec.reserve(num_points);

	std::vector<float> attr_vec;
	attr_vec.reserve(num_vertices);

	//	float maxX = 0;
	float minX = 0;
	float maxY = 0;
	float minY = 0;
	int bIndex = 0;

	double sum = 0;
	for (int i = 0; i < height; i++)
	{
		MZScan *scan = scans[i];
		// std::cout << "scan width = " << scan->getIntensity().size() << "  " << scan->getLcTime() << " \n";
		auto data = scan->getIntensity();
		for (auto x : data)
			sum += x;
	}

	mzFloat maxX = 0;

	MZScan * blankt = new MZScan(scans[0]);
	MZScan * blankb = new MZScan(scans[0]);

	blankt->blank();
	blankb->blank();

	//this stores the (next) line from where an x-position was succesfully written
	//so we can retrieve and share vertices
	//bearing in mind that some lines are missing vertices (due to zero removal)
	memset(last_pos1y, -1, sizeof(last_pos1y));

	for (int i = 0; i < height + 1; i++)
	{
		MZScan *top = blankt;

		MZScan *bottom = blankb;
		lcFloat y1, y2;
		if (i > 0)
		{
			top = scans[i - 1];
			y1 = top->getLcTime();
		}
		else
		{

			y1 = scans[0]->getLcTime() - .0001f;
		}

		if (i >= height)
		{

			//	if (tile->LOD > 3)
			{
				y2 = y1 + .000001f;
			}

		}
		else
		{
			bottom = scans[i];
			y2 = bottom->getLcTime();
		}


		auto topMz = top->getMz();
		auto botMz = bottom->getMz();

		if (type == MZDataType::square)
		{
			MZScan *zero = scans[0];

			topMz = zero->getMz();
			botMz = topMz;


		}

		int yp = (int)(((y1 + y2) / 2 - lcRange.min) * mapY / (lcRange.max - lcRange.min));


		auto topI = top->getIntensity();
		auto botI = bottom->getIntensity();

		y1 -= (lcRange.min + lcRange.max) / 2;
		y2 -= (lcRange.min + lcRange.max) / 2;

		y1 *= yScale;
		y2 *= yScale;


		int last = 0;
		unsigned short last_pos1;
		unsigned short last_pos2;
		int loops = 0;
		for (int j = 1; j < width; j++)
		{


			if (topI.size() <= j)
				break;
			if (botI.size() <= j)
				break;

			auto x1 = topMz[j - 1];


			maxX = std::max(x1, maxX);
			if (x1 <= 0)
			{
				std::cout << "-ve x \n";
				//				continue;
			}
			last = j;



			auto x2 = topMz[j];

			auto z1 = topI[j - 1];
			auto z3 = botI[j - 1];

			auto z2 = topI[j];
			auto z4 = botI[j];

			//	z3 = 0;
			//	z1 = 322221;
			//	z2 = 322221;
			//	z4 = 0;


			 

			total++;

			// don't do too many - long polygons can cause problemns with highlighting
			int lastj = std::min(width - 4, j + 4);

			while ((j < lastj) && (z1 == z2) && (z2 == topI[j + 1]) && (z3 == z4) && (z4 == topI[j + 1]))
			{
				numSkip++;

				total++;

				j++;

				x2 = topMz[j];
				z2 = topI[j];
				z4 = botI[j];

			}
 



			byte mapVal = 0;
			if (mapSet)
			{
				int xp = (int)((((x1 + x2) / 2) - mzRange.min) * mapX / (mzRange.max - mzRange.min));

				mapVal = map[xp / 8][yp];
				mapVal &= (1 << (xp & 7));

			}
 
			if (loops < 1)
			{


			 


				if (insertData(tile, vertex_vec, uv_vec, vb_vec, attr_vec, x1, x1, y1, y2, 0, z1, 0, z3, mapVal, 0))
				{

				}


				//always insert it, but don't add sides
				if (insertData(tile, vertex_vec, uv_vec, vb_vec, attr_vec, x1, x2, y1, y2, z1, z2, z3, z4, mapVal, 1))
				{
					bIndex += 6;
					inserted++;
					loops++;
					last_pos1y[j] = i + 1;

				}
			}
			else
			{
				//if the previous line drew in this position, we can reuse three points
				// (i.e., the top right matches the previous bottom right
				if (last_pos1y[j] == (i))
				{
					assert(j < max_line_size);

			 


					if (insertData(tile, vertex_vec, uv_vec, vb_vec, attr_vec, x2, y2, z4, last_pos1, last_pos2, last_pos2y[j], mapVal))
					{
						last_pos1y[j] = i + 1;

						loops++;
					}
				}
				else
				{
					////if it didn't, we only reuse the two (x) xpoints
					if (insertData(tile, vertex_vec, uv_vec, vb_vec, attr_vec, x2, y1, y2, z2, z4, last_pos1, last_pos2, mapVal))
					{
						last_pos1y[j] = i + 1;

						loops++;
					}

				}




			}
			assert(j < max_line_size);

			if (vb_vec.size() == 0)
			{
				loops = 0;
			}
			else
			{
				assert(vb_vec.size() > 5);

				last_pos1 = vb_vec[vb_vec.size() - 2];
				last_pos2 = vb_vec[vb_vec.size() - 1];
				last_pos2y[j] = vb_vec[vb_vec.size() - 1];

				assert(last_pos2y[j] <= vertex_vec.size());
			}


			//drop (immediataly) to zero to prevent gaps when adjacent tile is different LOD
			if (j >= width - 1)
			{
				if (1)

					if (insertData(tile, vertex_vec, uv_vec, vb_vec, attr_vec, x2, x2, y1, y2, z2, 0, z4, 0, mapVal, 0))
					{
						loops++;

					}

			}


		}

	}




	delete blankt;
	delete blankb;
	Mesh* mesh;
	if (attr_vec.size() > 0)
		mesh = new Mesh(vertex_vec, uv_vec, vb_vec, attr_vec);
	else
		mesh = new Mesh(vertex_vec, uv_vec, vb_vec);
	// std::cout << total << "  " << numSkip << "  " << inserted << "  " << (total-numSkip-inserted) << "\n";
//	std::cout << "#Num tris " << num_vertices << "  " << bIndex << " " << vertex_vec.size() << " sum = " << info.num_points << "\n";


	return mesh;
}



void Landscape::makeMeshProcess(Tile* tile)
{
	//	meshLock.lock();

	auto status = tile->drawStatus;
	Landscape* l = tile->owner;
	// reCreatingMesh



	auto dataTile = tile->getMZData();

	if (dataTile->size() == 0)
	{
		return;
	}

	// Mesh* lmesh = dataTile->makeMesh2(tile);
	Mesh* lmesh = l->makeMesh2(tile);

	if (lmesh != NULL)
	{
		//	lmesh->setTexture(peakTexture);
			//	lmesh->makeNormals();

		tile->setMesh(lmesh);
		//setMesh may set status normally, but do it here if the mesh is being recreated
		if (status == DrawStatus::reCreatingMesh)
		{
			tile->drawStatus = DrawStatus::reCreateGLMesh;
		}


	}
}



void Landscape::setMap()
{
 	mapSet = false;

	static std::atomic<int> setId = 0;
	setId++;
	int thisId = setId;

	memset(&map[0], 0, mapX*mapY / 8);

	mzFloat minX = worldMzRange.min;
	mzFloat rangeX = worldMzRange.max - minX;
	lcFloat minY = worldLcRange.min;
	lcFloat rangeY = worldLcRange.max - minY;

	//just because this is pointers
	auto annotations = getAnnotationsByName();

	float xs = Settings::hlFilterSize;
	float ys = Settings::hlFilterSize / 8;

	int size = (int)((xs * mapX / rangeX) + .5);
	size = std::max(size, (int)((ys * mapY / rangeY) + .5));


	for (auto a : annotations)
	{

		int ix = (int)(((a->mz - minX)*mapX / rangeX));
		int iy = (int)(((a->lc - minY)*mapY / rangeY));


		for (int i = -size; i <= size; i++)
			for (int j = -size; j <= size; j++)
			{

				//quits if a new thread  was called while still updating (due to slider control)
				if (thisId != setId)
					return;

				if ((i*i + j * j) > (size*size))
					continue;

				int x = ix + i;
				if ((x < 0) || x >= mapX)
					continue;

				int y = iy + j;
				if ((y < 0) || y >= mapY)
					continue;

				byte val = 1;
				map[x / 8][y] |= 1 << (x & 7);



			}

	}
 

	mapSet = true;

	//temp bodge - no attrivute rebuild
//	return;

	if (0)
		for (int i = 0; i < mapX; i++)
			for (int j = 0; j < mapY; j++)
			{
				map[i][j] = (i^j) & 1;
			}
	reBuild();
}

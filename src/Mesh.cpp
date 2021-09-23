#include "Structs.h"
#include "Mesh.h"
#include <iostream>

float Mesh::zscale = 1.0;
float Mesh::logzscale = 1.0;

int numMeshes[8] = { 0,0,0,0,0,0,0,0 };

Mesh::Mesh(std::vector<glm::vec3> vertex_vec, std::vector<glm::vec2> uv_vec, std::vector<glm::vec3> normal_vec,  GLTextureID texture_id)
{
	status = MeshStatus::noData;
	init(vertex_vec, uv_vec);
	normalVec.clear();
	normalVec = normal_vec;
	normal = &normalVec[0];
	texture = texture_id;
	status = MeshStatus::ready;
//	type = 0;
//	numMeshes[type]++;
 }

Mesh::Mesh(std::vector<glm::vec3> vertex_vec, std::vector<glm::vec2> uv_vec, std::vector<glm::vec3> normal_vec )
{
	
	status = MeshStatus::noData;
	init(vertex_vec, uv_vec);

	normalVec.clear();
	normalVec = normal_vec;
	normal = &normalVec[0];
	status = MeshStatus::ready;
//	type = 1;
//	numMeshes[type]++;

}


Mesh::Mesh(std::vector<glm::vec3> vertex_vec, std::vector<glm::vec2> uv_vec, std::vector<unsigned int> vb_vec, bool hd)
{
	//may need to deal with Meshes that have no attributes (but they won't normally have a vervec buffer)
	//assert(0);

	status = MeshStatus::noData;
	init(vertex_vec, uv_vec);


	vbVec.clear();
	vbVec = vb_vec;
	vb = &vbVec[0];
	_useVB = true;
	status = MeshStatus::ready;

	size = vbVec.size();

	if ((size % 6) == 0)
		makeNormals2();
	highDetail = hd;

//	type = 2;
//	numMeshes[type]++;
	
}



Mesh::Mesh(std::vector<glm::vec3> vertex_vec, std::vector<glm::vec2> uv_vec, std::vector<unsigned int> vb_vec, std::vector<float> attr_vec,  bool hd)
{

	status = MeshStatus::noData;
	init(vertex_vec, uv_vec);


	vbVec.clear();
	vbVec = vb_vec;
	vb = &vbVec[0];
	_useVB = true;


	attributeVec.clear();
	attributeVec = attr_vec;
	attribute = &attributeVec[0];

	status = MeshStatus::ready;

	size = vbVec.size();


	highDetail = hd;

	if ((size % 6) == 0)
		makeNormals2();
//	type = 3;
//	numMeshes[type]++;
}


 
void Mesh::init(std::vector<glm::vec3> vertex_vec, std::vector<glm::vec2> uv_vec)
{
	vertexVec = vertex_vec;
	uvVec = uv_vec;

	vertex = &vertexVec[0];
	uv = &uvVec[0];
	size = vertexVec.size();
	arraySize = vertexVec.size();


	std::vector<glm::vec3> out_normals;
	glm::vec3 faceNormal;
//just make some space to put the normals ?
 
	out_normals.reserve(size);
	for (unsigned int i = 0; i < size; i++)
		out_normals.push_back(faceNormal);

 	normalVec = out_normals;


	normal = &normalVec[0];

	if (( size % 6)==0)
 	makeNormals();

}

Mesh::Mesh(std::vector<glm::vec3> vertex_vec, std::vector<glm::vec2> uv_vec)
{
	//an empty mesh is now fine - removed by noise removal
	if (vertex_vec.size() < 1)
	{
//		std::cout << " error : empty mesh\n";
		return;
	}
	init(vertex_vec, uv_vec);
//	type = 4;
//	numMeshes[type]++;
}

Mesh::Mesh(std::vector<glm::vec3> vertex_vec, std::vector<glm::vec2> uv_vec,  GLTextureID texture_id)
{
	//an empty mesh is now fine - removed by noise removal
	if (vertex_vec.size() < 1)
	{
		return;
	}
	init(vertex_vec, uv_vec);
	texture = texture_id;
//	type = 5;
//	numMeshes[type]++;;
}

void Mesh::makeNormals()
{
 
	 
	for (int i = 0; i < size; i += 6)
	{
		glm::vec3 faceNormal = glm::normalize(glm::cross(vertex[i + 2] - vertex[i],
			vertex[i] - vertex[i + 1]));

		glm::vec3 faceNormal2 = glm::normalize(glm::cross(vertex[i + 2+3] - vertex[i + 3],
			vertex[i + 3] - vertex[i + 1 + 3]));


		glm::vec3 newNormal = (faceNormal + faceNormal2) *0.5f;

 
		normal[i] = newNormal;
		normal[i + 1] = faceNormal;
		normal[i + 2] = newNormal;

	//	newNormal = faceNormal2;

		normal[i + 3] = faceNormal2;
		normal[i + 4] = newNormal;
		normal[i + 5] = newNormal;


	}
 }

void Mesh::makeNormals2()
{
 
	for (unsigned int i = 0; i < size; i += 6)
	{

		glm::vec3 points[6];
		for (unsigned int j = 0; j < 6; j++)
		{
			assert( (i + j) <= vbVec.size());
			if (vb[i + j] > vertexVec.size())
			{
	//			std::cout << vb[i + j] << "  : " << vertexVec.size() << "\n";
			}
			assert(vb[i + j] <= vertexVec.size());
			points[j] = vertex[vb[i + j]];
		}


		glm::vec3 faceNormal = glm::normalize(glm::cross(points[2] - points[0],
			points[0] - points[1]));
#if 0
		glm::vec3 faceNormal2 = glm::normalize(glm::cross(points[2 + 3] - points[3],
			points[3] - points[1 + 3]));


		glm::vec3 newNormal = (faceNormal + faceNormal2) *0.5f;




		normal[vb[i]] = newNormal;
		normal[vb[i + 1]] = faceNormal;
		normal[vb[i + 2]] = newNormal;

		//	newNormal = faceNormal2;

		normal[vb[i + 3]] = faceNormal2;
		normal[vb[i + 4]] = newNormal;
		normal[vb[i + 5]] = newNormal;
#endif

		//if (normal[vb[i + 0]].x == -2)
		normal[vb[i + 0]] = faceNormal;

		//if (normal[vb[i + 3]].x == -2)
		normal[vb[i + 3]] = faceNormal;

	}
}

Mesh::~Mesh()
{	 
 

	/*
	for (int i=0; i < 5; i++)
	if (numMeshes[i] > 1)
	{//
		std::cout << " deleting " << normalVec.size() << "  " << numMeshes << "  " << type << "\n";

	//	std::cout << "?";
	}
	numMeshes[type]--;
	*/

}

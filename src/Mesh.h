#pragma once

#include "glm/glm.hpp"
#include <vector>
#include "GLTextureID.h"
#include "Settings.h"
// a generic (non-GL Specific) representation of a 'mesh' - i.e.; a piece of landscape to be drawn
// for version 1, used to create a GLMesh, which can actually be drawn 
// GLTextureID is an internal type - just represents a texture identification - likely to be an int

class Mesh
{
public:
	Mesh(std::vector<glm::vec3> vertex, std::vector<glm::vec2> uv, std::vector<glm::vec3> normal, GLTextureID texture);
	Mesh(std::vector<glm::vec3> vertex, std::vector<glm::vec2> uv, std::vector<glm::vec3> normal);
	Mesh(std::vector<glm::vec3> vertex_vec, std::vector<glm::vec2> uv_vec, std::vector<unsigned short> vb_vec , std::vector<float> attr_vec, bool highDetail = false);
	Mesh(std::vector<glm::vec3> vertex_vec, std::vector<glm::vec2> uv_vec, std::vector<unsigned short> vb_vec, bool highDetail = false);

	Mesh(std::vector<glm::vec3> vertex, std::vector<glm::vec2> uv, GLTextureID texture);
	Mesh(std::vector<glm::vec3> vertex, std::vector<glm::vec2> uv);

// 	Mesh(meshHandle h) { handle = h; }
	static inline float convertZ(signalFloat input) { 
//		if (Settings::logTransform)
//			return (float)std::sqrt(input) * logzscale;
		return (float) (input * zscale); }

	~Mesh();
	inline glm::vec3* getVertex() { return vertex; }
	inline glm::vec2* getUv() { return uv; }
	inline glm::vec3* getNormal() { return normal; }
	inline float* getAttribute() { return attribute; }

	inline unsigned short* getVB() { return vb; }


	bool useVB() { return _useVB; }
	inline GLTextureID getTexture() { return texture; }
	inline void setTexture(GLTextureID texture_id) { texture = texture_id; }
	inline int getSizeArray() { 		return (int) arraySize; 	}
	inline int getSize() { return (int)size; }

	void makeNormals();
	void makeNormals2();

	
	inline void testClear()
	{

		std::vector<unsigned short>().swap(vbVec);
		std::vector<float>().swap(attributeVec);
		std::vector<glm::vec3>().swap(normalVec);
		std::vector<glm::vec3>().swap(vertexVec);
		std::vector<glm::vec2>().swap(uvVec);
 
	}
 	static float logzscale;

	std::vector<unsigned short> vbVec;

	std::vector<float> attributeVec;
	std::vector<float> testVec;

	bool highDetail = false;

private:
	MeshStatus status = MeshStatus::hasHandle;
	bool _useVB = false;

	int type = 0;
 
	static float zscale;
	void init(std::vector<glm::vec3> vertex_vec, std::vector<glm::vec2> uv_vec);
 
 
	size_t size = 0;
	size_t arraySize = 0;
	glm::vec3* vertex = NULL;
	glm::vec2* uv = NULL;
	glm::vec3* normal = NULL;
	float* attribute = NULL;

	std::vector<glm::vec3> vertexVec;
	std::vector<glm::vec2> uvVec;
	std::vector<glm::vec3> normalVec;


	unsigned short* vb = NULL;


	GLTextureID texture;
};


#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdio.h>

#include "gl3w/gl3w.h" // Include glfw3.h after our OpenGL definitions
#include "glfw/include/GLFW/glfw3.h" // Include glfw3.h after our OpenGL definitions

#include "Structs.h"
#include "GLMesh.h"
#include "Render.h"
#include <iostream>
#include "Settings.h"
#include "Globals.h"

void setWireFrame()
{

}
int numGLMeshes = 0;
// #define DEBUG_GRAM 1

#ifdef DEBUG_GRAM

static int glMemVB = 0;
static int glMemVertex = 0;
static int glMem = 0;
static int glMemUV = 0;
static int glMemNormal = 0;
#endif


GLMesh::GLMesh(Mesh* m, GLMesh* pm)
{
	//if passed a parent GLmesh = that's the GLMesh created without the wireframe
	// if it's using a vbo, then it has all of the vertices we need, just have to share them
	// (primarily to save memory)



	drawObject.useVB = pm->drawObject.useVB;
	drawObject.normal = pm->drawObject.normal;
	drawObject.uv = pm->drawObject.vb;
	drawObject.vertex = pm->drawObject.vertex;


	glGenVertexArrays(1, &drawObject.vao);
	glBindVertexArray(drawObject.vao);


	glBindBuffer(GL_ARRAY_BUFFER, drawObject.vertex);
	glEnableVertexAttribArray(Render::vertexPosition_modelspaceID);
	glVertexAttribPointer(
		Render::vertexPosition_modelspaceID,  // The attribute we want to configure
		3,                            // size
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);

	// 2nd attribute buffer : UVs

	glBindBuffer(GL_ARRAY_BUFFER, drawObject.uv);
	glEnableVertexAttribArray(Render::vertexUVID);

	glVertexAttribPointer(
		Render::vertexUVID,                   // The attribute we want to configure
		2,                            // size : U+V => 2
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);

	glBindBuffer(GL_ARRAY_BUFFER, drawObject.normal);
	glEnableVertexAttribArray(Render::vertexNormal_modelspaceID);


	glVertexAttribPointer(
		Render::vertexNormal_modelspaceID,    // The attribute we want to configure
		3,                            // size
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);




	numGLMeshes++;



	wireframeShared = true;
	int size = pm->drawObject.size;

	int wsize = size * 4 / 6;  //for every quad (pair of triangles), we only need two lines to make the wireframe
	unsigned short* wirem = new unsigned short[wsize];
	unsigned short* vbb = m->getVB();
	glm::vec3* vp = m->getVertex();

	auto baseRemoval = Settings::baseRemoval;
	if ((Settings::showBaseWireframe) && (m->highDetail))
		baseRemoval = -1;


	int j = 0;
	for (int i = 0; i < size; i += 6)
	{
		//remove grid lines for very low triangles


		if ((vp[vbb[i]].y + vp[vbb[i + 1]].y + vp[vbb[i + 2]].y < baseRemoval))
		{
			wsize -= 4;

		}
		else
		{
			wirem[j++] = vbb[i];
			wirem[j++] = vbb[i + 4];
			wirem[j++] = vbb[i];
			wirem[j++] = vbb[i + 2];

		}

	}
	assert(j <= wsize);
	glGenBuffers(1, &drawObject.vb);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawObject.vb);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, wsize * sizeof(unsigned short), wirem, GL_STATIC_DRAW);

#ifdef DEBUG_GRAM
	glMem += wsize * sizeof(unsigned short);
	glMemVB += wsize * sizeof(unsigned short);
#endif

	drawObject.size = wsize;
	delete[] wirem;


	drawObject.texture = m->getTexture();

	glBindVertexArray(0);

	// delete m;
	// delete pm;
}

static float blank[] = { 0,0,0,0 };


GLMesh::GLMesh(Mesh* m, bool wireframe)
{
	numGLMeshes++;


	drawObject.useVB = m->useVB();

	GLuint buffers[8];
	int numBuffers = 4 + (drawObject.useVB == true);
	glGenBuffers(numBuffers, buffers);

	int cbf = 0;



	glGenVertexArrays(1, &drawObject.vao);
	glBindVertexArray(drawObject.vao);


	glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);



	drawObject.vertex = buffers[cbf++];

	glBindBuffer(GL_ARRAY_BUFFER, drawObject.vertex);



	auto vtx = m->getVertex();
	auto attrs = (m->getAttribute());

	// encoding the attribute as negative y position
   // to be resolved in vertex shader
		   // (to save memory)
		   // doesn't work with wireframe - plan b?
		   //encode into UV ? 
	if (1)
		if (m->attributeVec.size() > 1)
			for (int i = 0; i < m->getSizeArray(); i++)
			{
				auto x = std::abs(vtx[i].y);
				if (attrs[i] > 1.1)
				{
					x = -x;
				}
				vtx[i].y = x;

			}

	glBufferData(GL_ARRAY_BUFFER, m->getSizeArray() * sizeof(glm::vec3), m->getVertex(), GL_STATIC_DRAW);

	if (m->attributeVec.size() > 1)
		for (int i = 0; i < m->getSizeArray(); i++)
		{
			auto x = std::abs(vtx[i].y);
			vtx[i].y = x;
		}

#ifdef DEBUG_GRAM
	glMem += m->getSizeArray() * sizeof(glm::vec3);
	glMemVertex += m->getSizeArray() * sizeof(glm::vec3);
#endif

	glEnableVertexAttribArray(Render::vertexPosition_modelspaceID);

	glVertexAttribPointer(
		Render::vertexPosition_modelspaceID,  // The attribute we want to configure
		3,                            // size
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);







	drawObject.uv = buffers[cbf++];


	glBindBuffer(GL_ARRAY_BUFFER, drawObject.uv);
	// 2nd attribute buffer : UVs
	// glEnableVertexAttribArray(Render::vertexUVID);


	if ((drawObject.useVB) && (wireframe))
		glDisableVertexAttribArray(Render::vertexUVID);

	else
	{
		glBufferData(GL_ARRAY_BUFFER, m->getSizeArray() * sizeof(glm::vec2), m->getUv(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(Render::vertexUVID);
	}
#ifdef DEBUG_GRAM

	glMem += m->getSizeArray() * sizeof(glm::vec2);
	glMemUV += m->getSizeArray() * sizeof(glm::vec2);
#endif



	glVertexAttribPointer(
		Render::vertexUVID,                   // The attribute we want to configure
		2,                            // size : U+V => 2
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);









	drawObject.normal = buffers[cbf++];
	glBindBuffer(GL_ARRAY_BUFFER, drawObject.normal);
	glBufferData(GL_ARRAY_BUFFER, m->getSizeArray() * sizeof(glm::vec3), m->getNormal(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(Render::vertexNormal_modelspaceID);

#ifdef DEBUG_GRAM
	glMem += m->getSizeArray() * sizeof(glm::vec3);
	glMemNormal += m->getSizeArray() * sizeof(glm::vec3);
#endif


	// 3rd attribute buffer : normals
	glBindBuffer(GL_ARRAY_BUFFER, drawObject.normal);
	glVertexAttribPointer(
		Render::vertexNormal_modelspaceID,    // The attribute we want to configure
		3,                            // size
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);





	if (drawObject.useVB)
	{

		assert(m->vbVec.size() == m->getSize());

		drawObject.vb = buffers[cbf++];

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawObject.vb);
		//glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->getSize() * sizeof(unsigned short), m->getVB(), GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->getSize() * sizeof(unsigned short), m->getVB(), GL_STATIC_DRAW);
#ifdef DEBUG_GRAM
		glMem += m->getSize() * sizeof(unsigned short);
		glMemVB += m->getSize() * sizeof(unsigned short);
#endif

	}

	assert(cbf <= numBuffers);
	drawObject.texture = m->getTexture();
	drawObject.size = m->getSize();
	glBindVertexArray(0);

#ifdef DEBUG_GRAM

	std::cout << "GLmem = " << glMem / 1024LL / 1024 << "\n";
	std::cout << "GLmemVB = " << glMemVB / 1024LL / 1024 << "\n";
	std::cout << "GLmemvertex = " << glMemVertex / 1024LL / 1024 << "\n";

	std::cout << "GLmemUV = " << glMemUV / 1024 / 1024LL << "\n";
	std::cout << "glMemNormal = " << glMemNormal / 1024LL / 1024 << "\n";

	std::cout << Globals::currentTime.time << "\n";

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

	GLint total_mem_kb = 0;
	glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX,
		&total_mem_kb);

	GLint cur_avail_mem_kb = 0;
	glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX,
		&cur_avail_mem_kb);



	std::cout << " tg " << total_mem_kb / 1024 << "  rg " << cur_avail_mem_kb / 1024 << "\n";
	if (Globals::currentTime.time > 10e6)
	{
		std::cout << "?";

	}
#endif

}


GLMesh::~GLMesh()
{

	numGLMeshes--;

	// in this order, as other buffers referenceed from vb
	// also means wireframe should be dleted first
	glDeleteBuffers(1, &drawObject.vb);


	if (false == wireframeShared)
	{

		glDeleteBuffers(1, &drawObject.vertex);
		glDeleteBuffers(1, &drawObject.normal);
		glDeleteBuffers(1, &drawObject.uv);

	}

	glDeleteVertexArrays(1, &drawObject.vao);


}

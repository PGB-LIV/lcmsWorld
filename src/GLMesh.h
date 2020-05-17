#pragma once
#include "Mesh.h"
#include "GLDraw.h"

//GLMesh may be renderer-specific
//It takes a generic mesh (in standard pointer format), and can return a drawable object
//It should sort out GLBuffers etc.
//other (non-GL) drawable objects could be added later
class GLMesh
{
public:
	GLMesh(Mesh* mesh, bool wireframe);
	GLMesh(Mesh* m, GLMesh* pm);

	~GLMesh();

 
	inline GLDraw *getDrawObject() { return &drawObject; }

 
private:
//	Mesh* mesh;
	GLDraw drawObject;
	bool wireframeShared = false;


};


#pragma once

#include "gl3w/gl3w.h"  
#include "GLTextureID.h"

// Describes a 3d object to be drawn by the graphics system
// This is specific to the OpenGL / GLFW implementation
// (In the OPenlGL / GLFW implementation, GLuints are used as 'handles' to the buffers - they could otherwise be pointers etc.)

struct GLDraw
{
	GLuint normal = 0;
	GLuint vertex = 0 ;
	GLuint uv = 0;
	GLuint vb = 0;
 	GLuint vao = 0;
	GLTextureID texture = 0;
	unsigned int size = 0;
	bool useVB = false;
	float alpha;
};


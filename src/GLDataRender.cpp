#include "GLDataRender.h"



GLDataRender::GLDataRender()
{
}


GLDataRender::~GLDataRender()
{
}

//scales lctime to GL position (probably linear)
//should be inline for performance

float getY(lcFloat input)
{
	return (float) input;
}

//scales intensity to GL position (probably a variable function)
//should be inline for performance

float getZ(signalFloat input)
{
	return (float) input;
}


void GLDataRender::RenderMZData(MZData in)
{
	//need - vertices, normals, uvs
	//maybe create quad first?
	//do we know size in advance?  n-1 quads per line, m lines

	//lets do one line first



}
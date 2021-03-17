
#pragma once
#include <deque>
#include "glm/glm.hpp"
#include "Structs.h"
#include <vector>
#include <set>
class Landscape;
class GLMesh;
class Tile;

class Render
{


	static std::deque<DeferDraw> deferredQueue;



	static GLuint programID;
 	// Get a handle for our "MVP" uniform
	static GLuint MatrixID;
	static GLuint ViewMatrixID;
	static GLuint ModelMatrixID;
	static GLuint XFilterID;
	static GLuint YFilterID;
	static GLuint ZFilterID;
	static GLuint zHighlightFilterID;

	static float zHighlightFilterValue;


	

	// Load the texture
	static GLuint PlainTexture;
	static GLuint cubeTexture;
	static GLuint cubeTexture2;

	// Get a handle for our "myTextureSampler" uniform
	static GLuint TextureID;
	static GLuint AlphaID;
	static GLuint rootFlagID;
	static GLuint ambientID;



	static GLuint LightID;
	static GLuint LightPowerID;

	static float frameTime;


	static float zFilter;
	static std::set<int> drawn_tiles;


	static void drawMeshVB(GLDraw *drawObject, bool wireFrame);
	static void drawMesh(GLDraw *drawObject, bool wireFrame);
	static void loadTextures();


public:
	static void drawTarget();
	static GLuint getMatrixID() { return MatrixID; }
	static void  drawCubeMeshDirection(GLMesh* cube, int type);

	// Get a handle for our buffers
	static GLuint vertexPosition_modelspaceID;
	static GLuint vertexUVID;
	static GLuint attribID;
	static GLuint vertexNormal_modelspaceID;


	const static float highThreshold;
	static void readyTiles(Landscape *l);
	static void rebuildWireframe();
	static void drawCubeMesh(GLMesh* cube, int type);
	static void drawBaseMesh(bool wire);
	static void drawDeferred();
	static GLuint currentTexture;
	
	static GLuint Texture[16];
	
	static GLuint FolderTexture;
	static GLuint LcmsTexture;
	static GLuint CsvTexture;

	static glm::mat4 GlobalViewMatrix;
	static glm::mat4 GlobalProjectionMatrix;
	static glm::dmat4 FixedProjectionMatrix;

	static glm::mat4 ModelMatrix;


	static glm::mat4 ProjectionMatrix;
	static glm::mat4 ViewMatrix;
	static glm::mat4 MVP;

	static bool drawWireFrame;
	static glm::dmat4  prepareView(Landscape* l);


	static void drawTile(Tile* tile, bool isFading);

	static bool setup();
};

#include "gl3w/gl3w.h" // Include glfw3.h after our OpenGL definitions

#include <time.h>
#include <thread>
#include <fstream>
#include <atomic>
#include "shader.hpp"
#include "texture.hpp"
#include "Structs.h"
#include "Mesh.h"
#include "Landscape.h"
#include "Tile.h"
#include "GLMesh.h"
#include "MZLoader.h"
#include "MzmlLoader.h"
#include "RawLoader.h"
 #include "Utils.h"
#include <sstream>
#include <iomanip>

#include <cmath>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp" // after <glm/glm.hpp>

#include "SampleLoader.h"
#include "Camera.h"
#include "Error.h"
#include "Zip.h"
#include "Annotations.h"
#include "Render.h"
#include "SystemSetup.h"
#include "Input2.h"
#include "gui.h"

#define  DEPTH_OFFSET 20.0f 
const float Render::highThreshold = 0;

const float normalAmbientLevel = 0.7f;

const float flatAmbientLevel = 1.1f;

float ambientLevel = 0.9f;

std::deque<DeferDraw> Render::deferredQueue;

bool Render::drawWireFrame = true;

GLuint Render::programID;
 // Get a handle for our "MVP" uniform
GLuint Render::MatrixID;
GLuint Render::ViewMatrixID;
GLuint Render::ModelMatrixID;
GLuint Render::XFilterID;
GLuint Render::YFilterID;
GLuint Render::ZFilterID;
GLuint Render::zHighlightFilterID;

float Render::zHighlightFilterValue;

// Get a handle for our buffers
GLuint Render::vertexPosition_modelspaceID;
GLuint Render::vertexUVID;
GLuint Render::attribID;

 

GLuint Render::vertexNormal_modelspaceID;

// Load the texture
GLuint Render::Texture;
GLuint Render::PlainTexture;
GLuint Render::cubeTexture;
GLuint Render::cubeTexture2;
GLuint Render::FolderTexture;
GLuint Render::LcmsTexture;
GLuint Render::CsvTexture;
GLuint CentreTexture;

// texture handles
GLuint Render::TextureID;
GLuint Render::AlphaID;
GLuint Render::rootFlagID;
GLuint Render::ambientID;



GLuint Render::LightID;
GLuint Render::LightPowerID;


float Render::frameTime;

glm::mat4 Render::GlobalViewMatrix;
glm::mat4 Render::GlobalProjectionMatrix;
glm::mat4 Render::ModelMatrix;

glm::mat4 Render::ProjectionMatrix;
glm::mat4 Render::ViewMatrix;
glm::mat4 Render::MVP;
float Render::zFilter = 0;
std::set<int> Render::drawn_tiles;


static bool rebuildBaseMesh = false;

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


#include "../files/heatmap2.h"
#include "../files/wirecolour.h"
#include "../files/cube2.h"
#include "../files/folder.h"
#include "../files/lcms.h"
#include "../files/csv.h"
#include "../files/centre.h"

void  Render::loadTextures()
{
	//create textures from RAM
	Texture = loadBMP_custom_data(heatmap2);

	PlainTexture = loadBMP_custom_data(wirecolour);

	// #include "../files/cube.h"
	// 	cubeTexture = loadBMP_custom_data(cube);

	cubeTexture2 = loadBMP_custom_data(cube2);
	FolderTexture = loadBMP_custom_data(folder);
	LcmsTexture = loadBMP_custom_data(lcms);
	CsvTexture = loadBMP_custom_data(csv);
	CentreTexture = loadBMPA_custom_data(centre);
	
}

extern GLuint g_ShaderHandle;

GLuint testVAO;

bool Render::setup()
{

	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return false;

	// Decide GL+GLSL versions
#if __APPLE__
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
	 
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);            // 3.0+ only
 
#endif

	
	glfwWindowHint(GLFW_SAMPLES, 4);


	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_ALPHA_BITS, 8);
	glfwWindowHint(GLFW_STENCIL_BITS, 0);
	glfwWindowHint(GLFW_DEPTH_BITS, 32);


	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);


	int width = Settings::windowWidth;
	int height = Settings::windowHeight;
	if (width*height == 0)
	{
		width = 1100;
		height = 700;
	}




	// Create window with graphics context
	Globals::window = glfwCreateWindow(width, height, "lcmsWorld", NULL, NULL);
	if (Globals::window == NULL)
		return false;
	glfwMakeContextCurrent(Globals::window);
	
	glfwFocusWindow(Globals::window);

	bool err = gl3wInit() != 0;
 

	if (err)
	{
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return false;
	}


 
	std::cout << "Setup For Drawing - Compiling Shaders \n";

	const char* geom =
#include "../files/Geometry.txt"

#ifdef __APPLE__
		const char* vertex =
#include "../files/macvertex.txt"

		const char* fragment =
#include "../files/macfragment.txt"
	// Create and compile our GLSL program from the shaders

#else
		const char* vertex =
#include "../files/vertex.txt"

		const char* fragment =
#include "../files/fragment.txt"

#endif

	programID = CompileShaders(vertex,fragment, geom);
	//programIDWire = LoadShaders((Settings::loadPath + "files/vertex.txt").c_str(), (Settings::loadPath + "files/fragment.txt").c_str());
 
	std::cout << "Shaders loaded " << programID << "\n";

	XFilterID = glGetUniformLocation(programID, "xFilter");
	YFilterID = glGetUniformLocation(programID, "yFilter");
	ZFilterID = glGetUniformLocation(programID, "zFilter");

	zHighlightFilterID = glGetUniformLocation(programID, "zHighlightFilter");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	LightPowerID = glGetUniformLocation(programID, "LightPower");

	
	// Get a handle for our buffers
	vertexPosition_modelspaceID = glGetAttribLocation(programID, "vertexPosition_modelspace");
	vertexUVID = glGetAttribLocation(programID, "vertexUV");
	attribID = glGetAttribLocation(programID, "vertexAttribute");
	std::cout << " vertexAttribute " << attribID << "\n";
	std::cout << " vertexUV " << vertexUVID << "\n";

	vertexNormal_modelspaceID = glGetAttribLocation(programID, "vertexNormal_modelspace");

	std::cout << "GL Handles acquired " << vertexUVID << "\n";
	// Load the texture
	// Texture = loadDDS("files/uvmap.DDS");


	TextureID = glGetUniformLocation(programID, "TextureSampler");
	AlphaID = glGetUniformLocation(programID, "alpha");
	rootFlagID = glGetUniformLocation(programID, "rootFlag");
	ambientID = glGetUniformLocation(programID, "ambient");

 
	loadTextures();
	
	//	PlainTexture = loadBMPA_custom("wirecoloura.bmp");
	// Read our .obj file

	//	mesh = loadOBJFile("suzanne.obj");
	//	tileMesh->setTexture(Texture);



 
	// Get a handle for our "LightPosition" uniform
	glUseProgram(programID);

	glUniform1f(ambientID, ambientLevel);

		glfwGetFramebufferSize(Globals::window, &width, &height);
		glViewport(0, 0, width, height);
		glfwSwapInterval(1);




		gui::setup(glsl_version);



		glBindVertexArray(0);

		glfwFocusWindow(Globals::window);

		return true;

}

/* 
void testHUD()
{
	auto Ortho = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

}
*/

glm::mat4  Render::prepareView(Landscape* l)
{
 	
	auto clearColour = Settings::clearColour;


	glClearColor(clearColour.x, clearColour.y, clearColour.z, clearColour.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBlendFunc(GL_ONE, GL_ZERO);


	glCullFace(GL_FRONT);
 
	// might be better enabled
	glDisable(GL_CULL_FACE);
	glEnable(GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);


	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LEQUAL);

	// Use our shader
	glUseProgram(programID);

	// Compute the MVP matrix from keyboard and mouse input


	
	ProjectionMatrix = GlobalProjectionMatrix;
	ViewMatrix = GlobalViewMatrix;



	float zScale = Settings::scale.z;
	if (l != NULL)
	{
		if (Settings::transformType == sqrtTransform)
		{
			glUniform1i(rootFlagID, 1);

			zScale = (float)(zScale * std::sqrt(l->viewData.peakHeight));
			//for log transorm, se rooFlag to 2
				 // zScale = zScale * (MZData::viewData.peakHeight)/std::log2((MZData::viewData.peakHeight));

		}
		else if (Settings::transformType == logTransform)
		{
			glUniform1i(rootFlagID, 2);

			//for log transorm, se rooFlag to 2
			zScale = (float)(zScale * (l->viewData.peakHeight) / std::log2((l->viewData.peakHeight)));

		}
		else
		{
			glUniform1i(rootFlagID, 0);

		}

	}
	else
		glUniform1i(rootFlagID, 0);


	ModelMatrix = glm::mat4(1.0f);
 auto 	sm = glm::scale(glm::vec3(Settings::scale.x, zScale, Settings::scale.y));
//	ModelMatrix = glm::scale(glm::vec3(Settings::scale.x, zScale, Settings::scale.y));

 
 
	MVP = ProjectionMatrix * ViewMatrix  * ModelMatrix * sm;


 
	// 	std::cout << "set projection matrix " << ViewMatrix[0][0] << "\n";
		// Send our transformation to the currently bound shader, 
		// in the "MVP" input
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

// 		glm::vec3 lightPos = glm::vec3(50000, 50000, 1000);
//		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

#if 0
		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, PlainTexture);
		glActiveTexture(GL_TEXTURE0);
		// Set our "TextureSampler" sampler to user Texture Unit 0
#endif

 	glActiveTexture(GL_TEXTURE0);

	//ned to convert filter into worldspace

	std::vector<float> f = { -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), std::numeric_limits<float>::max() ,0 };
	if (l!= NULL)
		f = l->getFilterModelSpace();


	glUniform2f(XFilterID, f[0] - 1, f[1] + 1);
	glUniform2f(YFilterID, f[2] - 1, f[3] + 1);

	glUniform1f(ZFilterID, f[4] - .0001f);




	zFilter = f[4] - .0001f;

	return MVP;
}



inline void Render::drawMeshVB(GLDraw *drawObject, bool wireFrame)
{
	glBindVertexArray(drawObject->vao);
	glDrawElements(GL_TRIANGLES, drawObject->size, GL_UNSIGNED_SHORT, nullptr);

}


void Render::drawMesh(GLDraw *drawObject, bool wireFrame)
{
 	if (drawObject->useVB)
	{
		drawMeshVB(drawObject, wireFrame);
		return;
	}

	// 1rst attribute buffer : vertices
	//glEnableVertexAttribArray(0);
//	glEnableVertexAttribArray(1);
//	glEnableVertexAttribArray(2);

//	glDisableVertexAttribArray(4);

	glBindVertexArray(drawObject->vao);
 

#if 0
	glEnableVertexAttribArray(vertexPosition_modelspaceID);


	glBindBuffer(GL_ARRAY_BUFFER, drawObject->vertex);
	glVertexAttribPointer(
		vertexPosition_modelspaceID,  // The attribute we want to configure
		3,                            // size
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);

	glEnableVertexAttribArray(vertexUVID);
	// 2nd attribute buffer : UVs
	glBindBuffer(GL_ARRAY_BUFFER, drawObject->uv);
	glVertexAttribPointer(
		vertexUVID,                   // The attribute we want to configure
		2,                            // size : U+V => 2
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);


	glEnableVertexAttribArray(vertexNormal_modelspaceID);

	// 3rd attribute buffer : normals
	glBindBuffer(GL_ARRAY_BUFFER, drawObject->normal);
	glVertexAttribPointer(
		vertexNormal_modelspaceID,    // The attribute we want to configure
		3,                            // size
		GL_FLOAT,                     // type
		GL_FALSE,                     // normalized?
		0,                            // stride
		(void*)0                      // array buffer offset
	);

#endif
 

	if (wireFrame)
	{



		glm::vec3 lightPos = glm::vec3(0, -1e9, 00);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		//	glLineWidth(1);

		glDrawArrays(GL_LINES, 0, drawObject->size);




	}
	else
	{

		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0, DEPTH_OFFSET);


		glm::vec3 lightPos = glm::vec3(0, 72000, 0);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_BLEND);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);



		glDrawArrays(GL_TRIANGLES, 0, drawObject->size);
		glDisable(GL_POLYGON_OFFSET_FILL);


	}








}




void Render::rebuildWireframe()
{
	rebuildBaseMesh = true;
}

void Render::drawCubeMesh(GLMesh* cube, int type)
{
	glUniform1f(ZFilterID, -1);
	glUniform1f(zHighlightFilterID, -1);

	glDisable(GL_CULL_FACE);

	glDisable(GL_DEPTH_TEST);

	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_BLEND);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);



	//glEnable(GL_POLYGON_OFFSET_LINE);
	//glPolygonOffset(0, -100000);

	glBindTexture(GL_TEXTURE_2D, cubeTexture2);


	glUniform1f(AlphaID, 0.13f);

	

	auto drawObject = cube->getDrawObject();

	drawMesh(drawObject, false);
 	glDisable(GL_POLYGON_OFFSET_LINE);

}

//sets up uniforms after base / wireframe, but just before tiles
void Render::readyTiles(Landscape *l)
{
	if (l != NULL)
	{
 
		float ht = Settings::highlightFilter ? 1 : l->worldSignalRange.max ;

 
		zHighlightFilterValue = ht * Settings::scale.z * l->zScale;
 //no longer a user-controlled option - but leave code here in case it is re-enabled
		zHighlightFilterValue = 0;
 
		if (Settings::highlightFilter)
		
			glUniform1f(zHighlightFilterID, zHighlightFilterValue);
		else
			glUniform1f(zHighlightFilterID, 1e23f);



	}
	glUniform1f(ZFilterID, zFilter);


	glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);



	// 1rst attribute buffer : vertices






	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0, DEPTH_OFFSET);


	glm::vec3 lightPos = glm::vec3(0, 72000, 0);
	glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

	if (Settings::flatLighting)
		glUniform1f(LightPowerID, 0.0f);
	else
		glUniform1f(LightPowerID, 115.0f);




	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_BLEND);

	//various bits are currently wrong way round
	// e.g. - sides of full detail
	// and those added to get to 0
	//todo - sort these out so I can back face cull

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


	ambientLevel = normalAmbientLevel;
	if (Settings::flatLighting)
		ambientLevel = flatAmbientLevel;

	//	glBindTexture(GL_TEXTURE_2D, Texture);
		// Set our "myTextureSampler" sampler to user Texture Unit 0
		//glUniform1i(TextureID, 0);
	glUniform1f(ambientID, ambientLevel);

}
void Render::drawBaseMesh(bool wire)
{

	static GLMesh* base = NULL;
	static GLMesh* baseQ = NULL;

	if (wire)
		if (Settings::addGridLines == false)

			return;
	glUniform1f(ZFilterID, -1);
	glUniform1f(zHighlightFilterID, -1);

	if (rebuildBaseMesh)
	{

		if (base != NULL)
			delete base;
		base = NULL;
		rebuildBaseMesh = false;
	}

	if (base == NULL)
		if (System::primary != NULL)
		{
			base = System::primary->makeBaseMesh(PlainTexture);
			baseQ = System::primary->makeBaseQuads(Texture);
		}


	if (base != NULL)
	{
		if (wire)
		{


			glEnable(GL_POLYGON_OFFSET_LINE);
			glPolygonOffset(0, 0);

			auto drawObject = base->getDrawObject();
			glBindTexture(GL_TEXTURE_2D, PlainTexture);
			drawMesh(drawObject, true);

			glDisable(GL_POLYGON_OFFSET_LINE);


		}
		else
		{
			glUniform1f(AlphaID, 1.0f);


			auto drawObject = baseQ->getDrawObject();
			// std::cout << " draw base " << drawObject->size << "\n";

			glBindTexture(GL_TEXTURE_2D, Texture);
			drawMesh(drawObject, false);



		}


	}
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(0, 0);
	glUniform1f(ZFilterID, zFilter);

	glBindTexture(GL_TEXTURE_2D, Texture);


}




//This is something of a bodge - elemnts that are being faded must be drawn last
//they are drawn in reverse order, as the latest elements shold be the least detailed (i.e. largest)
// however it shouldn't really matter
void renderSetup();

//wireframe is drawn afterward - avoids v. frequent GL state changes
std::vector<GLDraw *> wireBuffer;

void Render::drawTarget()
{
	if (Settings::drawTarget < 0.05f)
		return;

#if 1

		float width = (float) Settings::windowWidth;
		float height = (float) Settings::windowHeight;

		static float lastWidth = 0;
		static float lastHeight = 0;
		glm::mat4 projection = glm::ortho(
			0.0f,
			static_cast<float>(width),
			static_cast<float>(height),
			0.0f,
			0.0f,
			100.0f
		);

		// 	std::cout << "set projection matrix " << ViewMatrix[0][0] << "\n";
			// Send our transformation to the currently bound shader, 
			// in the "MVP" input
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &projection[0][0]);
		glBindTexture(GL_TEXTURE_2D, CentreTexture);

		static GLMesh* t = NULL;


		if (t == NULL)
		{
			float f = width / 2;
			float x = width / 32;
			float g = height / 2;
			float z = height / 32;

			x = std::max(x, z);
			z = x;


			float h = 0;
			float y = 0;
			std::vector<glm::vec3> vertex_vec;
			std::vector<glm::vec2> uv_vec;

			float one = 0.99f;
			float zero = 0.01f;

			vertex_vec.push_back(glm::vec3(f - x, g - z, h - y));
			vertex_vec.push_back(glm::vec3(f + x, g + z, h + y));
			vertex_vec.push_back(glm::vec3(f - x, g + z, h + y));
			uv_vec.push_back(glm::vec2(one, one));
			uv_vec.push_back(glm::vec2(zero, zero));
			uv_vec.push_back(glm::vec2(one, zero));

			vertex_vec.push_back(glm::vec3(f - x, g - z, h - y));
			vertex_vec.push_back(glm::vec3(f + x, g - z, h + y));
			vertex_vec.push_back(glm::vec3(f + x, g + z, h + y));
			uv_vec.push_back(glm::vec2(one, one));
			uv_vec.push_back(glm::vec2(zero, one));
			uv_vec.push_back(glm::vec2(zero, zero));



			Mesh *m = new Mesh(vertex_vec, uv_vec);
			t = new GLMesh(m, false);
			delete m;

		}
		glDisable(GL_CULL_FACE);

		glDisable(GL_DEPTH_TEST);
		glUniform1f(AlphaID, Settings::drawTarget/100.0f );

		drawMesh(t->getDrawObject(), 0);
		glUniform1f(AlphaID, 1.0);

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);


		if ((lastHeight != height) || (lastWidth != width))
		{
			delete(t);
			t = NULL;
		}
	
#endif
}

void Render::drawDeferred()
{

//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, Texture);


	while (deferredQueue.size() > 0)
	{
		DeferDraw dd = deferredQueue.back();
		deferredQueue.pop_back();

		glUniform1f(AlphaID, dd.alpha);
		drawMesh(dd.drawObject, false);
	}

	glUniform1f(AlphaID, 1.0);

	glUniform1f(ambientID, 0.2f);
//	glUniform1i(TextureID, 0);
	glUniform1i(LightPowerID, 0);

	//		glBindTexture(GL_TEXTURE_2D, PlainTexture);

	glm::vec3 lightPos = glm::vec3(0, -1e9, 00);
	glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
 

	for (auto drawObject : wireBuffer)
	{
		glUniform1f(AlphaID, drawObject->alpha);
		glBindVertexArray(drawObject->vao);
		glDrawElements(GL_LINES, drawObject->size, GL_UNSIGNED_SHORT, nullptr);
 	}
	glUniform1f(AlphaID, 1.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glUniform1f(ambientID, ambientLevel);
	
	
 
	wireBuffer.clear();



}




void Render::drawTile(Tile* tile, bool isFading)
{



	GLMesh* glMesh = tile->getGLMesh();

	double time = Input::averageTime;

	float alpha = 1.0;
	if (isFading == false)
	{
		//are actually drawing it..
		if (tile->getScreenSize() > 0.07)
			tile->fader.reset();

	}
	else
	{
		tile->fader.process(time);
		if (tile->fader.isVisible() == false)
			return;
		alpha = tile->fader.getAlpha();
	}


	//	if (tile->drawStatus != ready)
	//		return;
	if (glMesh == NULL)
	{
		std::cout << "null mesh " << tile->id << "\n";

		return;
	}

 
 
#if 0
	if (drawn_tiles.count(tile->id) == 0)
	{
		drawn_tiles.insert(tile->id);

		// mesh may be invalidated
		//	std::cout << " new tile : " << tile->id << " id - s " << tile->getMesh()->getSize() << " " << drawn_tiles.size() << " : \n ";

	}
#endif


	// std::cout << " draw tile : " << tile->id << " id - s " << tile->getMesh()->getSize() << " " << drawn_tiles.size() << " : \n ";


	GLDraw *drawObject = glMesh->getDrawObject();

	// should be off-screen
	if (tile->getScreenSize() < 0)
	{
		 
		return;
	}
	if (alpha < 1.0f)
	{
		DeferDraw dd = { alpha, drawObject };
		deferredQueue.push_back(dd);
		return;
	}
	else
	{
 
		drawMesh(drawObject, false);

 
	}

	//	if (tile->getScreenSize() > .05)
	if (tile->getwGLMesh() != NULL)
	if (drawWireFrame)
	{
		alpha = (float) std::min(.6, tile->getCameraDistance());

		if (alpha > 0.05)
		{
			drawObject = tile->getwGLMesh()->getDrawObject();
			drawObject->alpha = alpha;
			wireBuffer.push_back(drawObject);
 		}

	}


}


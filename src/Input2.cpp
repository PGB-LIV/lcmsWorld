#include "gl3w/gl3w.h" // Include glfw3.h after our OpenGL definitions
#include <iostream>
#include <thread>
#include <random>
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
// #include <GL/gl3w.h>    

#include <cmath>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp" // after <glm/glm.hpp>

#include "SampleLoader.h"
#include "Camera.h"
#include "Error.h"
#include "Zip.h"
#include "Annotations.h"
#include "SystemSetup.h"
#include "Input2.h"
#include "Render.h"
#include "gui.h"
#include "ConcurrentMap.h"

static const double CLICK_TIME = 0.20 * 1e6;
static const double DOUBLE_CLICK_TIME = 0.50 * 1e6;
static const float clickZoomDistance = 65;

TimeStamp lastTouchTime;


const float accel = 2000.0;



static char keyMask[GLFW_KEY_LAST+1] = { 0 };

static double last_xpos, last_ypos;
void resetClick()
{
	double xpos, ypos;
	glfwGetCursorPos(Globals::window, &xpos, &ypos);
	last_xpos = xpos;
	last_ypos = ypos;
}


const static float aspect = 1.0f; // 16.0f / 9;

int Input::mouse_button_state = 0;
double Input::mouse_wheel_pos = 0;
//extern int mouse_button_state;
//extern double mouse_wheel_pos;

bool Input::remoteCursorOn = false;
DataPoint Input::remoteCursor = { 0,0,0 };


TimeStamp Input::lastPress[8];
TimeStamp Input::lastClick[8];
// Initial position : on +Z
 

static glm::vec3 initialPosition = glm::vec3(0, 38700, 5000);
glm::vec3 Input::position = initialPosition;
// Initial horizontal angle : toward -Z7
float Input::horizontalAngle = 3.14f;
// Initial vertical angle : none
float Input::verticalAngle = 0.0f;
// Initial Field of View
float Input::initialFoV = 01.0f;
bool Input::drawWireFrame = true;
std::deque<double> Input::frameTimes;
int Input::frameCount = 60;
double Input::averageTime = 0;



glm::vec3  Input::cursor3d;
glm::vec2 Input::cursor2d;



 int Input::mFBSize[2];
int Input::mSize[2];

float Input::mPixelRatio = 1;
DataPoint Input::clickedPoint;
DataPoint savedPoint;
DataPoint Input::cursorPoint;
DataPoint Input::lastCursor;


void Input::update() 
{
	// glfwGetTime is called only once, the first time this function is called
	static double lastTime = glfwGetTime();
	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = (float)(currentTime - lastTime);
	System::frameTime = deltaTime;

	frameTimes.push_back(currentTime - lastTime);
	if (frameTimes.size() > frameCount)
		frameTimes.pop_front();

	double total = 0;
	for (double delt : frameTimes)
		total += delt;
	averageTime = total / frameTimes.size();

	if (getView() == NULL)
		return;
	if (getView()->getCamera() == NULL)
		return;
	

	auto c = getView()->getCamera();
	mouseWheelFriction(deltaTime);
	//c->wheelMove(mouse_wheel_pos, deltaTime);
	if (abs(mouse_wheel_pos) > .0001)
		c->dragZoom(mouse_wheel_pos/400, deltaTime);



	double mv = 0.35;
	double mxt = 0;
	double myt = 0;

	int bs = 1 << GLFW_MOUSE_BUTTON_1;

	int keys[] = { GLFW_KEY_LEFT,GLFW_KEY_RIGHT ,GLFW_KEY_UP ,GLFW_KEY_DOWN };
	for (auto key : keys)
	{
		double mx = 0;
		double my = 0;
		int mods = keyMask[key] - 1;
		if (mods < 0)
			continue;
	
		if (key == GLFW_KEY_LEFT)
			mx = -mv;
		if (key == GLFW_KEY_RIGHT)
			mx = mv;
		if (key == GLFW_KEY_UP)
			my = mv;
		if (key == GLFW_KEY_DOWN)
			my = -mv;

		if (mods & GLFW_MOD_SHIFT)
		{
			bs = 1 << GLFW_MOUSE_BUTTON_2;
			mx = -mx;
		}

		if (mods & GLFW_MOD_CONTROL)
		{
			mouseWheelMove(my, mx);

			Settings::zScale_slider += (float) my * deltaTime * 5.0f;
			mx = 0;
			my = 0;
	 
		}

		mxt += mx;
		myt += my;

	}
	int status = 0;
	if ((keyMask[GLFW_KEY_LEFT_ALT] > 0)
		|| (keyMask[GLFW_KEY_RIGHT_ALT] > 0))
	{
		status = 1;
	}
 
	mouseDragged(bs, mxt*deltaTime, myt*deltaTime ,.5,.75, status);
	gui::setSliderValues();







	getView()->updateCamera(deltaTime);

	lastTime = currentTime;

}


void Input::computeViewMatrices() {


	if (getView() == NULL)
		return;
	if (getView()->getCamera() == NULL)
		return;

	if (std::isnan(position.x))
	{
		position = initialPosition;
	}



	// Get mouse position
	double xpos, ypos;
	glfwGetCursorPos(Globals::window, &xpos, &ypos);


	auto vp = getView()->getViewport();


	float FoV = initialFoV;// - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires setting up a callback for this. It's a bit too complicated for this beginner's tutorial, so it's disabled instead.

 //.0411

	auto cam = getView()->getCamera();

	// Settings::farPlane = 400000.0f;
	// if (getView()->getCamera()->get != NULL)
//  compress += (float)mouse_wheel_pos / 64;
	Settings::farPlane = 300000.0f;
		Settings::nearPlane = 7.0f;

		if (1)
		if (cam->distance < 50000)
		{
			Settings::farPlane /= 2;
			Settings::nearPlane /= 2;
		}

 

		if (1)
			if (cam->distance < 5000)
			{
				Settings::farPlane /= 2;
				Settings::nearPlane /= 2;
			}
					   // Projection matrix : 45 Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	Render::GlobalProjectionMatrix = glm::perspective(FoV / aspect, aspect, Settings::nearPlane, Settings::farPlane);

 


	Render::GlobalViewMatrix = getView()->getCameraMatrix();
	remoteCursorOn = false;

	if (Settings::enableRemoteControl)
	{
		if (processInControl())
		{
			Settings::remoteCamera = false;
			std::string cam_string = getView()->getCamera()->to_string(lastCursor);
			setCameraString(cam_string);
		}
		else
		{
			std::string cam_string = getCameraString();
			if (cam_string.length() > 16)
			{
				Settings::remoteCamera = true;
				remoteCursorOn = true;
				remoteCursor = getView()->getCamera()->from_string(cam_string);
			}

		}
	}
	else
		Settings::remoteCamera = false;

	// For the next frame, the "last time" will be "now"
}


#include "imgui/imgui_impl_glfw.h"

void Input::touch_callback(GLFWwindow* window, int button, int action, double x, double y)
{
 
}

void Input::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)

{//if it's a touch screen, last cursor may not be anywhere near click...

 	resetClick();

	//this should do something else, like cause the click to happen on the next frame
	//handlecursor is unreliable at this point, lack of z-buffer
	if (Globals::currentTime.time - lastTouchTime.time > 0.3 * 1000)
		handleCursor(getView());

 


 	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);


	if (action == GLFW_PRESS)
		lastPress[button & 7] = Globals::currentTime;
	takeControl();




	if (action == GLFW_RELEASE)
	{

		


		if (Globals::currentTime.time - lastPress[button & 7].time < CLICK_TIME)
		{

			if (Globals::currentTime.time - lastClick[button & 7].time < DOUBLE_CLICK_TIME)
				mouseDoubleClicked(button);
			else
				mouseClicked(button);

			lastClick[button & 7].time = Globals::currentTime.time;
		}


	}

	if (action == GLFW_PRESS)
		mouse_button_state |= 1 << button;
	if (action == GLFW_RELEASE)
		mouse_button_state &= ~1 << button;

}



inline glm::vec4  Input::transform(glm::vec3 input, Landscape *l)
{
	//todo
	//owner dereferencing should be removed, and values cached



	auto y1 = input.y;
	glm::vec4  result;
	y1 -= (getView()->worldLcRange.min + getView()->worldLcRange.max) / 2;


	y1 *= l->yScale;

	auto x1 = input.x;
	x1 -= (getView()->worldMzRange.min + getView()->worldMzRange.max) / 2;
	x1 *= (mzFloat)l->xScale;


	auto z1 = input.z;
	z1 *= l->zScale;
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



void Input::mouseWheelMove(double xoffset, double yoffset)
{
 	if (Settings::disableCamera)
		return;

	mouse_wheel_pos += yoffset * 1.5;


	if (getView() == NULL)
		return;


}

void Input::mouseWheelFriction(double deltaTime)
{

	double wheel_friction = -mouse_wheel_pos * deltaTime * 3;
	mouse_wheel_pos += wheel_friction;

}
void Input::mouseDragged(int buttonState, double mx, double my, double xp, double yp, int status)
{
	if (getView() == NULL)
		return;
	if (getView()->getCamera() == NULL)
		return;
	if ((mx == 0) && (my == 0))
		return;

	if (Settings::disableCamera)
		return;

	//disable click if was being dragged

	if (mx*mx + my * my > .001)
		for (int i = 0; i < 8; i++)
			lastPress[i] = TimeStamp{ 0.0 };


	if (Settings::dragMode)
	{
		//flip status of first two buttons
		buttonState ^= 3;
	}
 
	if ((buttonState & 2) )
	{
		getView()->getCamera()->dragTranslate(mx, my, System::frameTime, status);

		return;
	}

	if (std::abs(my) > std::abs(mx))
	{
		//getView()->getCamera()->dragZoom(my, System::frameTime);
		getView()->getCamera()->wheelMove(my*400, System::frameTime);
		//c->wheelMove(mouse_wheel_pos, deltaTime);

	}
	else
	{
	
		double ys = yp - .5;
		double mn = .1;
		if (ys >= 0)
			if (ys < mn)
				ys = mn;
		if (ys < 0)
			if (ys > -mn)
				ys = -mn;
		ys = ys * 4.25;

		
		getView()->getCamera()->dragRotate(mx / (ys), System::frameTime);

	}



}

float Input::checkMouseNearTarget(Landscape *l)
{

	DataPoint t = l->getCamera()->getTarget();



	glm::vec4  p1 = transform({ t.mz, t.lc, t.signal }, l);
	auto tp = Render::MVP * p1;
	tp = tp / (tp.w);

	auto view = l->getViewport();

	float px = ((tp.x + 1)*view.x / 2);


	float py = ((-tp.y + 1)*view.y / 2);

	float dx = px - cursor2d.x;
	float dy = py - cursor2d.y;

	return std::sqrt((dx*dx + dy * dy));



}

void Input::mouseClicked(int button)
{
	if (Settings::disableCamera)
		return;


	if (button != 0)
		return;


	clickedPoint = cursorPoint;
	savedPoint = clickedPoint;

	//no longer used - it turned out to be awful to use
	return;

	Landscape *l = getView();

	if (getView() == NULL)
		return;
	if (getView()->getCamera() == NULL)
		return;
	if (cursor2d.y < 20)
		return;
	if (last_xpos > Settings::windowWidth - gui::infoWidth)
		return;

	if (last_ypos > Settings::windowHeight - (gui::statusHeight*1.5))
		return;
	if (gui::inMenu())
		return;

	bool isCentre =  (checkMouseNearTarget(l) < clickZoomDistance);

	getView()->getCamera()->updateCameraTarget(cursorPoint, 0, button, isCentre);
	clickedPoint = cursorPoint;

}

void Input::mouseDoubleClicked(int button)
{


	if (button != 0)
		return;

	if (Settings::disableCamera)
		return;

	if (getView() == NULL)
		return;
	if (getView()->getCamera() == NULL)
		return;
 

	if (cursor2d.y < 20)
		return;


	 

	getView()->getCamera()->updateCameraTarget(clickedPoint, 1, button, false);

}




void Input::handleCursor(Landscape* l)
{
	if (getView() == NULL)
		return;

	int col = 1;



	lastTouchTime = Globals::currentTime;


	getCursorZ();

	auto vp = getView()->getViewport();
	bool blankCursor = false;
	if (cursor3d.z >= 1)
	{
		cursor3d.z = .9999999f;
		blankCursor = true;
	}


	float posX, posY, posZ;

	float maxZ = .9999999999f;
	float minZ = .6f;

	int lp = 0;
	do
	{



		glm::vec4 vp4 = glm::vec4(0.0, 0.0, (GLdouble)vp.x, (GLdouble)vp.y);
		float zScale = Settings::scale.z;

	 
		auto 	sm = glm::scale(glm::vec3(Settings::scale.x, zScale, Settings::scale.y));

		auto xyz = glm::unProject(cursor3d, sm, Render::ProjectionMatrix*Render::ViewMatrix, vp4);

		posX = xyz.x;
		posY = xyz.z;
		posX /= l->xScale;
		posX += (getView()->worldMzRange.min + getView()->worldMzRange.max) / 2;

		posY /= l->yScale;
		posY += (getView()->worldLcRange.min + getView()->worldLcRange.max) / 2;

		posZ = xyz.y;
		posZ /= l->zScale;
		posZ += (getView()->worldLcRange.min + getView()->worldLcRange.max) / 2;


		cursorPoint.mz = posX;
		cursorPoint.lc = posY;
		cursorPoint.signal = posZ;

		lp++;

		if (cursorPoint.signal < 0)
		{
			maxZ = cursor3d.z;
			cursor3d.z = (cursor3d.z + minZ) / 2;
		}

		else
		{
			minZ = cursor3d.z;
			cursor3d.z = (cursor3d.z + maxZ) / 2;
		}

		//move closer to the camera if getting a -ve signal position


	} while (blankCursor && ((cursorPoint.signal < 0) || (cursorPoint.signal > 100)) && (lp < 16));

	
	auto cursor = getView()->findDataPoint(posX, posY, posZ);


	if (remoteCursorOn)
	{
		cursor = remoteCursor;
		cursorPoint = cursor;

	}
	//	getView()->addMarker(cursorPoint.mz, cursorPoint.lc, cursorPoint.signal, 0, ".", 8, 26, 0);


	if (cursor.signal <= 0)
		cursor = lastCursor;
	else
		lastCursor = cursor;


	if (cursor.signal > 0)
	{
		if (Settings::showInfoPanel)

		{

			int col = 1;
			if (checkMouseNearTarget(l) < clickZoomDistance)
				col = 1;

			if (mouse_button_state == 0)
				getView()->addMarker(cursor.mz, cursor.lc, cursor.signal, col, "x", 8, 26, 0);
	 
		//	ImGui::Text("");
	 
			getView()->addInfo("<i>m/z");
			getView()->addInfo("<c>= " + std::to_string(cursor.mz));

	 

		//	getView()->addInfo("m/z = " + std::to_string(cursor.mz));

			float mins;
			float seconds = std::modf((float)cursor.lc, &mins);

			seconds *= 60;

			float part = seconds - (int)seconds;
			part = part * 10;



			std::ostringstream infolc;
			infolc << "RT = " << (int)mins << " min " << std::setfill('0') << std::setw(2) << (int)(seconds) << "." << (int)part << " s";

			getView()->addInfo(infolc.str());

			std::ostringstream info;
			info << "intensity = " << std::scientific << std::setprecision(2) << cursor.signal;
			getView()->addInfo(info.str());


		}

		if (mouse_button_state == 0)

			if (getView()->hasAnnotations())
				if (Settings::showNearest)
					showNearest(cursorPoint);
	}






}



void Input::getCursorZ()
{
	if (getView() == NULL)
		return;
	auto vp = getView()->getViewport();
	double xsize = (double)vp.x;
	double ysize = (double)vp.y;


	double xpos, ypos;
	glfwGetCursorPos(Globals::window, &xpos, &ypos);

	double m_x = xpos - last_xpos;
	double m_y = ypos - last_ypos;



	m_x /= xsize;
	m_y /= ysize;

	int status = 0;
	if ((keyMask[GLFW_KEY_LEFT_ALT] > 0)
		|| (keyMask[GLFW_KEY_RIGHT_ALT] > 0))
	{
		status = 1;
	}
	if (mouse_button_state)
	{
 		mouseDragged(mouse_button_state, m_x, m_y, xpos/xsize,ypos/ysize, status);

	}


	last_xpos = xpos;
	last_ypos = ypos;

	GLdouble winX, winY;

	const int xs = 5, ys = 5;
	GLfloat winZ[xs*ys];
	winX = (GLdouble)xpos;
	winY = (GLdouble)(ysize - ypos);  // Subtract The Current Mouse Y Coordinate 

	cursor2d = glm::vec2(winX, winY);


	double minZ = 1;
	 
			glReadPixels((int)(winX -xs/2), (int)(winY -ys/2), xs, ys, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ[0]);//Reads the depth buffer

			for (int i=0; i < xs*ys; i++)
			minZ = std::min(minZ, (double)winZ[i]);
		
	cursor3d = glm::vec3(winX, winY, minZ);
	 
}


void Input::showNearest(DataPoint cursorPoint)
{

	// Settings::showNearest
	static Annotation last;

	Annotation a = getView()->getClosestAnnotation(cursorPoint);
	// -ve is a signal that nothing was found nearby
	   // for cosmetic reasons, drawing the last is reasonable
	if (a.mz < 0)
		a = last;
	else
		last = a;

	if (a.mz > -1)
	{




		static TimeStamp lastChange;

		static std::string lastText;
		std::string text = a.shortText;


		if (text != lastText)
		{
			if (Globals::currentTime.time - lastChange.time < 1e5)
			{

				text = lastText;
			}
			else
				lastChange = Globals::currentTime;
		}



		lastText = text;












		getView()->addMarker((float)a.mz, (float)a.lc, 0, 1, text, a.width, a.height, Settings::cubeSize);

		getView()->addInfo("");
		getView()->addInfo("Closest Annotation");
		getView()->addInfo(a.text);

		if (a.accession.size() > 0)
			getView()->addInfo(a.accession);
		if (a.ptm.size() > 0)
			getView()->addInfo("ptm: " + a.ptm);

		getView()->addInfo("<i>m/z");
		getView()->addInfo("<c>= " + std::to_string(a.mz));
		std::ostringstream infolc;


		float mins;
		float seconds = std::modf((float)a.lc, &mins);
		seconds = seconds * 60;
		float part = seconds - (int)seconds;
		part = part * 10;

		infolc << "RT = " << (int)mins << "'" << std::setfill('0') << std::setw(2) << (int)(seconds) << "." << (int)part; "\"";

		getView()->addInfo(infolc.str());

		//		getView()->addInfo("lc = " + std::to_string((int)((a.lc * 600)/10)));
		std::ostringstream info2;

		if (a.signal > 0)
		{
			info2 << "intensity = " << std::scientific << std::setprecision(2) << a.signal;
			getView()->addInfo(info2.str());
		}

	}
}

void Input::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Input::mouseWheelMove(xoffset, yoffset);
	gui::scrollWheel(window, xoffset, yoffset);
	
}
bool reloadFile();


void Input::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
 
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	
	if (key >= GLFW_KEY_LAST)
		return;
	if (key < 0)
		return;
	 
	if (action == GLFW_PRESS)
		if (key==GLFW_KEY_F5)
			reloadFile();

 


	if (action == GLFW_REPEAT)
		keyMask[key] = mods + 1;

	if (action == GLFW_PRESS)
		keyMask[key] = mods+1;

	if (action == GLFW_RELEASE)
	{
		update();
		keyMask[key] = 0;
	}
	return;


}

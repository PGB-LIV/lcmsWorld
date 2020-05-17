#pragma once

#include "Structs.h"
#include "glm/glm.hpp"
#include <deque>
class Landscape;

class Input
{

	static bool remoteCursorOn;
	static DataPoint remoteCursor;


	static int mouse_button_state;
	

	static	TimeStamp lastPress[8];
	static TimeStamp lastClick[8];

	// Initial position : on +Z

	static glm::vec3 position ;

	// Initial horizontal angle : toward -Z7
	static float horizontalAngle ;
	// Initial vertical angle : none
	static float verticalAngle ;
	// Initial Field of View
	static float initialFoV ;




	static bool drawWireFrame ;

	
	static std::deque<double> frameTimes;
	static int frameCount ;

	

	static glm::vec3 cursor3d;
	static glm::vec2 cursor2d;

	static double mouse_wheel_pos ;
	static int mFBSize[2];
	static int mSize[2];

	static float mPixelRatio ;
	static DataPoint clickedPoint;
	static DataPoint cursorPoint;
	static DataPoint lastCursor;

	static inline Landscape* getView() {
		return System::primary;
	}

	static inline glm::vec4  transform(glm::vec3 input, Landscape *l);
	static void mouseWheelMove(double xoffset, double yoffset);
	static void mouseWheelFriction(double deltaTime);
	static void mouseDragged(int buttonState, double mx, double my, double xp, double yp, int shiftStatus);
	static float checkMouseNearTarget(Landscape *l);
	static void mouseClicked(int button);
	static void mouseDoubleClicked(int button);
	static void getCursorZ();
public:
	static double averageTime;
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void touch_callback(GLFWwindow* window, int button, int action, double x, double y);

	static void computeViewMatrices();
	

	static void handleCursor(Landscape* l);

	static void showNearest(DataPoint cursorPoint);

	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void update();

};

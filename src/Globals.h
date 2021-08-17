#pragma once
#include "Structs.h"
#include <chrono>
#include <array>
//possibly should be called 'system'
//just to make a few things faster to access from anywhere; e.g., time stamps
//don't want to make function calls every time, once per frame is plenty
#include "gl3w/gl3w.h"  
#include "glfw/include/GLFW/glfw3.h" // Include glfw3.h after our OpenGL definitions

#define TOC_VERSION true


class Globals
{
public:

#if TOC_VERSION 
	static inline std::string x_axis_desc = "prec. m/z";
	static inline std::string y_axis_desc = "frag. m/z";
	static inline std::string x_axis_name = "Prec.";
	static inline std::string y_axis_name = "Frag.";
#else
	static inline std::string x_axis_desc = "m/z";
	static inline std::string y_axis_desc = "RT";
	static inline std::string x_axis_name = "m/z";
	static inline std::string y_axis_name = "RT";
#endif


	static GLFWwindow* window;
	static std::array<std::string, 5> supported_versions; 
	static TimeStamp currentTime;
	static std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

	static bool firstSet; // will be set to false

	static std::string statusText;

	static std::vector<std::string> script;
	static void setCurrentTime() {
		if (firstSet == false)
		{
			firstSet = true;
			startTime = std::chrono::high_resolution_clock::now();
		}
		auto now = std::chrono::high_resolution_clock::now(); 

		std::chrono::duration<double,std::micro> diff = now - startTime;

		currentTime.time = diff.count();
 
			
	}

	static bool closing;

	static inline TimeStamp getCurrentTime()
	{
		return currentTime;
	}
	static long long loopCount;
	static std::string lastFileName;

	static void setFileStatus(std::string filename);
	static float mouseSpeed;
	static float wheelSpeed;

	static int windowWidthActive;
	static double windowScale;
};
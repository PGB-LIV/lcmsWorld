#pragma once
#include "Structs.h"
#include <chrono>

//possibly should be called 'system'
//just to make a few things faster to access from anywhere; e.g., time stamps
//don't want to make function calls every time, once per frame is plenty
#include "gl3w/gl3w.h"  
#include "glfw/include/glfw3.h" // Include glfw3.h after our OpenGL definitions

class Globals
{
public:
	static GLFWwindow* window;

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

	static inline TimeStamp getCurrentTime()
	{
		return currentTime;
	}
	static long long loopCount;
	static std::string lastFileName;

	static void setFileStatus(std::string filename);
	static float mouseSpeed;
	static float wheelSpeed;
};
#include "gl3w/gl3w.h" // Include glfw3.h after our OpenGL definitions

#include "Globals.h"
#include "Splash.h"
#include <chrono>
#include <thread>
#include "../files/splash.h"
#include "texture.hpp"
#include "Render.h"

#include "Settings.h"
#include "SystemSetup.h"
#include "Input2.h"
int width = 1920/2;
int height = 1080/2;
int twidth = 800;
int theight = 640;

Splash* Splash::instance = NULL;
 
GLuint splashtexture = 0;
GLuint framebuffer = 0;
bool Splash::Render()
{
	if (finished)
		return false;


 
	auto clearColour = Settings::clearColour;

	glfwMakeContextCurrent(window);
	
	if (splashtexture == 0)
	{
		glGenFramebuffers(1, &framebuffer);
		auto header = splash;
		twidth = *(int*)&(header[0x12]);
		theight = *(int*)&(header[0x16]);
		splashtexture = loadBMP_custom_data(splash);
	}
 
 
	 

		glClearColor(clearColour.x, clearColour.y, clearColour.z, clearColour.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	 
 
 
		glBindTexture(GL_TEXTURE_2D, splashtexture);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, splashtexture, 0);
 
		int pos_x = 0;
		int pos_y = 0;
		int dest_x0 = pos_x;
		int dest_x1 = pos_x + width;
		int dest_y0 = pos_y;
		int dest_y1 = pos_y + height;

		glBlitFramebuffer(pos_x, pos_y, twidth, theight, dest_x0, dest_y0, dest_x1, dest_y1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		

		
		glfwSwapBuffers(window);


 
	auto time = Globals::currentTime.time - startTime.time;

 
	if ((time > (3.5*1000*1000)) || (Input::last_click_time.time > startTime.time) ||  (glfwWindowShouldClose(window)) )
	{
		glDeleteFramebuffers(1, &framebuffer);
		finished = true;
		glfwDestroyWindow(window);
		return false;
	}

	return true;
}



Splash::Splash()
{
	using namespace std::chrono_literals;
	

	if (!glfwInit())
		return ;

	instance = this;
	 

 

 
	
	
		glfwWindowHint(GLFW_FLOATING, true);
		window = glfwCreateWindow(width, height, "ToCmsWorld Starting", NULL, NULL);
		

		glfwSetMouseButtonCallback(window, Input::mouse_button_callback);
		startTime = Globals::currentTime;



	 
	 
}
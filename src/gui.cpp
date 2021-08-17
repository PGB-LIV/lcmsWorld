#define NOMINMAX


#include "gl3w/gl3w.h" 
#include "glfw/include/GLFW/glfw3.h" // Include glfw3.h after our OpenGL definitions

#include <functional> 
#include <thread>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <ostream>
#include <fstream>
#include <string>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"

#include <ctime>  
#include <iomanip>
 

 
#include "gui.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include "Settings.h"
#include "Landscape.h"
#include "Utils.h"
#include "Camera.h"
#include "Error.h"
#include "SampleLoader.h"
#include "ImGuiFileDialog.h"
#include "SystemSetup.h"
#include "Input2.h"
#include "Render.h"
#include "Cache.h"
#include "Annotations.h"
#include "RawLoader.h"


#if TOC_VERSION
	#include "gui_toc.cpp"
#else
	#include "gui_lcms.cpp"
#endif


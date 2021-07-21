// FINAL switches off debug window and logs to text file
// should be set by compiler options
// #define FINAL

#define NOMINMAX
#include "gl3w/gl3w.h" // Include glfw3.h after our OpenGL definitions
#include "glfw/include/GLFW/glfw3.h" // Include glfw3.h after our OpenGL definitions
#include <functional> 
#include <thread>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <ostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_internal.h"
#ifdef _WIN32
#include <filesystem>
#endif

 
#include "ConcurrentMap.h"
#include "Landscape.h"
#include "SampleLoader.h"
#include "Settings.h"
#include "ImGuiFileDialog.h"
#include "Utils.h"
#include "LCHttp.h"

#include "Error.h"
#include "Annotations.h"

#include "Render.h"
#include "SystemSetup.h"
#include "Input2.h"
#include "gui.h"
#include "Builder.h"
#include "Render.h"
#include "Cache.h"
#include "Camera.h"

//  remove command prompt window
#ifdef FINAL
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#else

#endif
 

 
//needed to support multiple views in same executable, but this is unlikely to be supported in the near future
static Landscape* getView() { return System::primary; }

void checkSize()
{
	int width, height;
	static int last_width = 0, last_height = 0;
	glfwGetFramebufferSize(Globals::window, &width, &height);


	Settings::windowHeight = height;
	Settings::windowWidth = width;
	

	if (Settings::showInfoPanel)
	{
		width = std::max(gui::infoWidth, width - gui::infoWidth);
	}
	Globals::windowWidthActive = width;

	//setting a scale value so that amount of detail can change
	//depending on the window size
	double maxW = std::max(height, width);
	maxW /= 1700;

	Globals::windowScale = maxW;


	if ((width != last_width) || (height != last_height))
		glViewport(0, 0, width, height);

	if (getView() != NULL)
		getView()->updateViewport(width, height);

	last_width = width;
	last_height = height;

}


 


void reportError(std::string message, int severity)
{
	new Error(Error::ErrorType::file, message);

}


bool fileOpen(std::string filePathName)
{
	

	rtrim(filePathName);

	if (FILE *file = fopen(filePathName.c_str(), "r")) 
	{
		fclose(file);
	}
	else {
		Globals::statusText = "File not found " + filePathName;
		reportError("File not found");
		return false;
	}
//	Globals::statusText = "Loading " + filePathName;

	std::string loadFile = tolower(filePathName);


	if (endsWith(loadFile, ".mztab"))
	{
		Settings::lastAnnotationFilename = filePathName;

		Annotations::loadMZTab(filePathName, getView());
		return true;
	}
	if (endsWith(loadFile, ".mzid"))
	{
		Settings::lastAnnotationFilename = filePathName;

		Annotations::loadMZID(filePathName, getView());
		return true;
	}
	if (endsWith(loadFile, "mzid.gz"))
	{
		Settings::lastAnnotationFilename = filePathName;

		Annotations::loadMZID(filePathName, getView());
		return true;
	}
	if (endsWith(loadFile, ".csv"))
	{
		Settings::lastAnnotationFilename = filePathName;

		Annotations::loadCSVA(filePathName, getView());
		return true;
	}
	if (endsWith(loadFile, ".txt"))
	{
		Settings::lastAnnotationFilename = filePathName;

		Annotations::loadTextA(filePathName, getView());
		return true;
	}
	Settings::lastFilename = filePathName;

	if (endsWith(loadFile, ".lcms"))
	{
	//	std::cout << " load .lcms " << filePathName << "\n";
		Settings::lastAnnotationFilename = ".";
		Builder::makeLandscapeFromCache(filePathName);
		if (getView() != NULL)
			Globals::setFileStatus(filePathName);
		return (getView() != NULL);
	}

	if (endsWith(loadFile, ".mzml"))
	{
		Settings::lastAnnotationFilename = ".";
		std::thread t1(Builder::makeLandscape, filePathName);
		t1.detach();

		Globals::setFileStatus(filePathName);

		return (getView() != NULL);
	}

	//check for a registered extension
	std::string exe = gui::getFileReader(loadFile);

	if (endsWith(loadFile, ".raw") || (exe.length() > 0))
	{
		Settings::lastAnnotationFilename = "";
		std::thread t1(Builder::makeLandscape, filePathName);
		t1.detach();

		Globals::setFileStatus(filePathName);

		return (getView() != NULL);
	}
	reportError("Unknown file type");

	Globals::statusText = "Not Loading unknown file type " + filePathName;
	return true;
}

bool restartFlag = false;
void restart()
{
	restartFlag = true;
}


bool reloadFile()
{
	
	if (getView() != NULL)
	{
		restart();
		return false;
	}

	if (Settings::lastFilename.length() > 2)
	{
		auto annotation = Settings::lastAnnotationFilename;
		bool res = fileOpen(Settings::lastFilename);

		if (res)
		if (annotation.length() > 2)
			fileOpen(annotation);

		std::string camString = Settings::lastCameraString;
		if (camString.length() > 10)
		{
			
			std::replace(camString.begin(), camString.end(), '_', '\n');
			std::cout << "Set camera " << camString << "\n";
			if (getView() != NULL)
			getView()->getCamera()->from_string(camString);
		}


 

		return res;
	}
	return false;
}
bool isLoading = false;
void backgroundLoad(std::string line)
{
#ifdef _WIN32
	std::string  folder = std::filesystem::temp_directory_path().string();
#else
	std::string  folder = "/tmp/";
#endif
 

	if (folder.length() == 0)
		folder = "."+sep;
	isLoading = true;
	auto data = LCHttp::getFileFromHttp(line);
 	std::ofstream metaFile;

	std::string ext = line.substr(line.find_last_of("."));
	std::string name = folder+"webcache" + ext;
	metaFile.open(name , std::ios::out | std::ios::binary);
	metaFile.write((const char *)&data[0], data.size());
	metaFile.close();
	fileOpen(name);
	isLoading = false;
}
void processScript(std::vector<std::string> script)
{
	if (isLoading)
		return;
	//first line of script should be url to get metadata from
	//for ease of programming, writing it to a temporary file for now
 	

	static std::string command = "";
	static int position = 0;
	static std::string laf;
	static std::string lf;

	//will save and restorr last filename, as it will get overwritten during loading
	if (position == 0)
	{
		laf = Settings::lastAnnotationFilename;
		lf = Settings::lastFilename;
	}
	if (position >= script.size())
		return;

	std::string line = script[position];
	position++;

	{
		
		if (command == "load")
		{
			isLoading = true;
			std::thread t1(backgroundLoad, line);
			t1.detach();
		
			command = "";
 		}
		else if (command == "zoom")
		{
 
			std::stringstream l(line);
			DataPoint clickedPoint;
			
			l >> clickedPoint.mz;
			l >> clickedPoint.lc;
			l >> clickedPoint.signal;

 			getView()->getCamera()->updateCameraTarget(clickedPoint, 2, 0, false);


			//zoom to annotation
			command = "";
		}
		else if (command == "annotation")
		{
			if (getView()->hasAnnotations() == false)
			{
				position--;
				return;
			}
			auto a = getView()->getAnnotations();
			getView()->setVisible(line, true, true);

			command = "";
		}
		else if (command == "text")
		{
			//zoom to annotation
			Globals::statusText = line;
			command = "";
		}
		else
		{
			command = line;
		}


	}

	if (position >= script.size())
	{
		Settings::lastAnnotationFilename = laf;
		Settings::lastFilename = lf;
	}
	


}


void checkAutoLoad()
{
	if (Globals::script.size() > 0)
	{
		processScript(Globals::script);

 


	}

	if (Settings::autoLoadFile.length() > 0)
	{

		
	
		std::string filePathName = Settings::autoLoadFile;
 		Settings::autoLoadFile = "";

		if (filePathName == "-restart")
		{
			std::cout << " reloading \n" << Settings::lastFilename <<" \n";

			

			reloadFile();
			return;
		}



 
		fileOpen(filePathName);
	}
}

char* executable_name = NULL;

void restartNow()
{
	restartFlag = false;

	Globals::closing = true;
	Cache::closeCache();

	Settings::saveSettings();
	concurrent_end();



	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();


	glfwDestroyWindow(Globals::window);
	glfwTerminate();


	LCHttp::finish();

	//give threads a little more time to exit cleanly
	//
				//		resetView();
#ifdef _WIN32
	char pathtofile[MAX_PATH];

	GetModuleFileName(GetModuleHandle(NULL), pathtofile, sizeof(pathtofile));
#else

	if (executable_name == NULL)
	{
		exit(0);
		return;
	}

	char pathtofile[PATH_MAX];
	strcpy(pathtofile, executable_name);
#endif

	strcat(pathtofile, " -restart");
	system(pathtofile);
	// (pathtofile);
	//system(pathtofile, SW_SHOW);
	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	exit(0);

}
void exitApp()
{
	Globals::closing = true;
	Cache::closeCache();
	
	Settings::saveSettings();
	concurrent_end();
 
	

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();


 	glfwDestroyWindow(Globals::window);
	glfwTerminate();


	LCHttp::finish();

	//give threads a little more time to exit cleanly
	//
	std::this_thread::sleep_for(std::chrono::milliseconds(250));


	exit(0);
}



void drawLoop(glm::dmat4 view)
{
	if (getView() != NULL)
	{
		
		Render::drawBaseMesh(false);


		glClear( GL_DEPTH_BUFFER_BIT);
		Render::drawBaseMesh(true);

	//	glEnable(GL_CULL_FACE);

		Render::readyTiles(getView());
		getView()->drawTiles();

		getView()->updateLandscape(view);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		getView()->drawAnnotations();

		getView()->drawCubes();

		glEnable(GL_DEPTH_TEST);


	}
	else
	{

		// clear screen...
	}

}




void loadBMP_custom_data_ARGB(const unsigned char* imageData, GLFWimage &dest) {




	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	// Actual RGB data
	const unsigned char* data;


	// Open the file



	// Read the header, i.e. the 54 first bytes

	memcpy(header, imageData, 54);
	// A BMP files always begins with "BM"
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		return ;
	}
	// Make sure this is a 24bpp file
	//if (*(int*)&(header[0x1E]) != 0) { printf("Not a correct BMP file\n");    return; }
	//if (*(int*)&(header[0x1C]) != 32) { printf("Not a correct BMP file\n");    return ; }

	// Read the information about the image
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);
	std::cout << "texture " << width << " * " << height << std::endl;

	dest.width = width;
	dest.height = height;

	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width * height * 4; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way
	data = imageData + dataPos;

	dest.pixels = (unsigned char*)malloc(imageSize);
	for (unsigned int i = 0; i < imageSize; i += 4)
	{
		dest.pixels[i] = data[i+2];
		dest.pixels[i+1] = data[i + 1];
		dest.pixels[i+2] = data[i + 0];
		dest.pixels[i+3] = data[i + 3];
	}

 
	// Read the actual data from the file into the buffer

	 
}

//glfwSetWindowIcon sets them upside-down , was quicker to flip it at source
#include "../files/lcms64.h"
#include "../files/lcms32.h"

int cmain(int  argc, char ** argv)
{

	if (argc > 0)
		executable_name = argv[0];

#ifdef FINAL
#ifdef _WIN32
	AllocConsole();
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
	std::ofstream cout("errorlog.txt");
	std::cout.rdbuf(cout.rdbuf());

#endif
 


	std::set_terminate([]() {
		std::exception_ptr eptr;

		eptr = std::current_exception(); // capture

		std::cout << "Unhandled exception\n";
		try
		{
			std::rethrow_exception(eptr);
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
		std::abort(); 
		
		});




	std::cout << sizeof(size_t) * 8 << " bit version.\n";


	Settings::setup(argc, argv);
	Settings::loadSettings();

	System::setup();
	if (Render::setup() == false)
	{
		std::cerr << "Fatal Error setting up rendering system\n lcmsWorld needs an OpenGL 3+ compatible graphics card.\n";
#ifdef _WIN32
		MessageBox(nullptr, TEXT("Fatal Error setting up the rendering system.\nlcmsWorld needs an OpenGL 3+\
 compatible graphics card.\n\nThis may not be available on remote desktops or virtual machines."), TEXT("Error"), MB_OK);
#endif
		// exitApp();
		return 1;
	}

	std::thread t1(LCHttp::start);
	t1.detach();
	concurrent_start();
	
	GLFWimage iconImage[2];
	loadBMP_custom_data_ARGB(lcms32, iconImage[0]);
	loadBMP_custom_data_ARGB(lcms64, iconImage[1]);
 
	glfwSetWindowIcon(Globals::window, 2, iconImage);

	//If window is not focused, callbacks fail
	glfwFocusWindow(Globals::window);

	std::cout << glfwGetWindowAttrib(Globals::window, GLFW_FOCUSED) << " focused \n";

	while (!glfwGetWindowAttrib(Globals::window, GLFW_FOCUSED))
	{
		std::cout << "Window not focused\n";
		// window has input focus
		glfwFocusWindow(Globals::window);
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
	}

 

	glfwSetMouseButtonCallback(Globals::window, Input::mouse_button_callback);
 
	glfwSetScrollCallback(Globals::window, Input::scroll_callback);

 
	glfwSetKeyCallback(Globals::window, Input::key_callback);

//	glfwSetTouchCallback(Globals::window, Input::touch_callback);

 
	// Main loop
 	while (!glfwWindowShouldClose(Globals::window))
	{
		Globals::loopCount++;
  		checkSize();
		if (getView() != NULL)
		getView()->setInfo();


		Input::update();
		Input::computeViewMatrices();
		Input::handleCursor(getView());

		auto matrixView = Render::prepareView(getView());
 

		gui::guiLoop(matrixView);
		
		drawLoop(matrixView);

		glDisable(GL_DEPTH_TEST);

	
		gui::decorations();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(Globals::window);



		glEnable(GL_DEPTH_TEST);
		checkAutoLoad();
		if (restartFlag)
			restartNow();
		
	}

 
	// Cleanup
	exitApp();
 

	return 0;
}

int main(int  argc, char** argv)
{
	return cmain(argc,argv);
}
 



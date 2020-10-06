#include "Settings.h"
#include <algorithm>
#include <string>
#include <fstream>

#include "Structs.h"
#include "ImGuiFileDialog.h"
#include "SystemSetup.h"
#include "Camera.h"
#include "sys/stat.h"

bool Settings::flatLighting = false;
bool Settings::displaySides = true;
float Settings::detail = 5;

 float Settings::hlFilterSize = 3;
bool Settings::highlightFilter = false;
bool Settings::dragMode = false;

bool Settings::showBaseWireframe = false;
 
ImVec4 Settings::clearColour = ImVec4(120 / 255.0f, 131 / 255.0f, 144 / 255.0f, 1.00f);
std::string  Settings::loadPath = ".\\";
#ifdef _WIN32
std::string  Settings::lastFilename = ".";
#else
std::string  Settings::lastFilename = "/";
#endif

vec3<float> Settings::scale = { 1,1,1 };
bool Settings::showNumbers = false;
bool Settings::addGridLines = false;

float Settings::xScale_slider = 0;
float Settings::yScale_slider = 0;
float Settings::zScale_slider = 0;
//updating the version number will invalidate the settings file
std::string Settings::version = CUR_VERSION_STRING;

float Settings::wheelSpeed = 50;
float Settings::mouseSpeed = 50;

float Settings::drawTarget = 0;

bool Settings::disableCamera = false;
bool Settings::showInfoPanel = false;
std::vector<std::string > Settings::transformTypes{ "Linear", "Square Root", "Log" };
int Settings::transformType = 0;

std::string Settings::iniFilePath =".\\";
std::string Settings::logFilePath =".\\";
std::string Settings::iniFileName = "lcms_gui.ini";
std::string Settings::logFileName = "lcms_gui_log.txt";

bool Settings::experimentalMzml = false;
const static std::string settingsFileName = "lcms_prefs.ini";

std::string Settings::lastAnnotationFilename;
bool Settings::showNearest = true;
float Settings::nearPlane = 7.0f;
float   Settings::farPlane = 1000000.0f;
float   Settings::cubeSize = 60.0f;
bool Settings::mergeIdents = false;
int Settings::numViewAnnotations = 100;
int Settings::maxViewAnnotations = 300;

float Settings::baseRemoval = 3.5;
int Settings::windowWidth = 0;
int Settings::windowHeight = 0;
 
bool Settings::enableRemoteControl = false;
bool Settings::remoteCamera = false;
std::string Settings::autoLoadFile;
bool Settings::expertMode = false;
int Settings::axisMarker = 1;

std::string Settings::lastCameraString = "";
void Settings::setup(int  argc, char ** argv)
{
	if (argc == 0)
		return;

	std::string path(argv[0]);
	size_t found = path.find_last_of("/\\");
	loadPath = path.substr(0, found + 1);
	iniFilePath = loadPath + iniFileName;
	logFilePath = loadPath + logFileName;

 
	if (argc > 1)
	{
		std::string loadFile(argv[1]);
		autoLoadFile = loadFile;

 		if (argv[1][0] == '-')
			loadPath = "";



	}
	else
	{
 
		//assume we started in the same folder anyway
		loadPath = "";
 

	}

#ifdef _WIN32
	
#else
	// we can only write to certain locations
	const char* homeDir = getenv("HOME");
	char final[265 * 2];
	sprintf(final, "%s/.lcmsWorld/", homeDir);
	mkdir(final, S_IRUSR | S_IWUSR | S_IXUSR);
	loadPath = std::string(final);

	iniFilePath = loadPath + iniFileName;
	logFilePath = loadPath + logFileName;


#endif



	//	return example_main(argc,argv);

	std::cout << " folder: " << loadPath << "\n";


}

 

void Settings::loadSettings()
{
	std::ifstream settings(loadPath + settingsFileName);
	std::string version;
	
	std::cout << " Load settings\n";

	settings >> version;

	bool supported = false;
	for (const std::string& allv : Globals::supported_versions)
		if (version == allv)
			supported = true;

	if (!supported)
		return;

	std::getline(settings, lastFilename);

	std::getline(settings, lastFilename);



 	settings >> showBaseWireframe;

 	settings >> showNumbers;
	settings >> addGridLines;
	settings >> xScale_slider;
	settings >> yScale_slider;
	settings >> zScale_slider;

	settings >> clearColour.x;
	settings >> clearColour.y;
	settings >> clearColour.z;
	settings >> showInfoPanel;
	settings >> transformType;
	settings >> cubeSize;
	settings >> showNearest;
	
	std::string path;
 
	std::getline(settings, path);

	std::cout << "P:" << path << "\n";
	std::getline(settings, path);
	std::cout << "P:" << path << "\n";
	std::getline(settings, lastAnnotationFilename);

 

	ImGuiFileDialog::Instance()->setCurrentPath(path);

	if (lastFilename.length() > 1)
		if (endsWith(lastFilename, ".lcms") == false)
			lastFilename = lastFilename + ".lcms";

	settings >> numViewAnnotations;
	settings >> mergeIdents;

	settings >> windowWidth;
	settings >> windowHeight;

	settings >> enableRemoteControl;


	settings >> hlFilterSize;
	settings >>  highlightFilter;
	settings >> detail;
	settings >> displaySides;
	settings >> flatLighting;
	settings >> experimentalMzml;
	settings >> drawTarget;
	settings >> dragMode;
	settings >> expertMode;
	settings >> wheelSpeed;
	settings >> mouseSpeed;
	settings >> axisMarker;
	settings >> lastCameraString;
	setMouse();
 

	settings.close();












	float xScale = Settings::xScale_slider;
	float yScale = Settings::yScale_slider;
	float zScale = Settings::zScale_slider;

	Settings::scale.x = xScale < 0 ? -1 / (xScale - 1) : xScale + 1;
	Settings::scale.y = yScale < 0 ? -1 / (yScale - 1) : yScale + 1;
	Settings::scale.z = zScale < 0 ? -1 / (zScale - 1) : zScale + 1;
	Settings::scale.z = std::pow(Settings::scale.z, 1.55f);

}

void Settings::saveSettings()
{
	std::ofstream settings(loadPath + settingsFileName);
 
	lastCameraString = "";
	if (System::primary != NULL)
	{
		auto camera = System::primary->getCamera();
		if (camera != NULL)
		{
			DataPoint d = { 0, 0, 0 };

			std::string camString = camera->to_string(d);

			std::replace(camString.begin(), camString.end(), '\n', '_');
			lastCameraString = camString;
		}

	}


	settings << version << "\n";
	std::string name = lastFilename;
	if (name.length() == 0)
		name = ".";

	settings << name << "\n";
	settings << showBaseWireframe << "\n";
 	settings << showNumbers << "\n";
	settings << addGridLines << "\n";
	settings << xScale_slider << "\n";
	settings << yScale_slider << "\n";
	settings << zScale_slider << "\n";

	settings << clearColour.x << "\n";
	settings << clearColour.y << "\n";
	settings << clearColour.z << "\n";
	settings << showInfoPanel << "\n";

	settings << transformType << "\n";
	settings << cubeSize << "\n";
	settings << showNearest << "\n";
	std::string path = ImGuiFileDialog::Instance()->getCurrentPath();
	if (path.length() == 0)
		path = ".";

	settings << path << "\n";
	name = lastAnnotationFilename;
	if (name.length() == 0)
		name = ".";
	settings << name << "\n";
	settings << numViewAnnotations << "\n";
	settings << mergeIdents << "\n";

	settings << windowWidth << "\n";
	settings << windowHeight << "\n";

	settings << enableRemoteControl << "\n";
	settings << hlFilterSize << "\n";
	settings << highlightFilter	<< "\n";
	settings << detail << "\n";
	settings << displaySides << "\n";
	settings << flatLighting<< "\n";
	settings << experimentalMzml << "\n";
	settings << drawTarget << "\n";
	settings << dragMode << "\n";
	settings << expertMode << "\n";
	settings << wheelSpeed << "\n";
	settings << mouseSpeed << "\n";
	settings << axisMarker << "\n";
	settings << lastCameraString << "\n";
	
	settings.close();

}


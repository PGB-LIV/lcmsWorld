#pragma once
#include "Structs.h"
#include "Globals.h"
#include "imgui/imgui.h"
#include <string>
#include <vector>
#include "glm/glm.hpp"


class Settings
{
public:
	//adjusts the amount of detail shown, constant setting
	

#if TOC_VERSION
	static inline float xScale_slider_max = 30;
	static inline float xScale_slider_min = -30;
	static inline float DetailConstant = .0004f;  //was .0005
#else
	static inline float xScale_slider_max = 10;
	static inline float xScale_slider_min = -10;
	static inline float DetailConstant = .0005f;  
#endif

	static void setMouse() { 
		if (Settings::mouseSpeed >= 50)
			Globals::mouseSpeed = 1+ (Settings::mouseSpeed-50.0f)/5.0f;
		else
			Globals::mouseSpeed = -0.9f/(Settings::mouseSpeed - 50.0f) + .1f;
		 
		if (Settings::wheelSpeed >= 50)
			Globals::wheelSpeed = 1 + (Settings::wheelSpeed - 50.0f) / 5.0f;
		else
			Globals::wheelSpeed = -0.9f / (Settings::wheelSpeed - 50.0f) + .1f;

		//Globals::wheelSpeed = Globals::wheelSpeed * Globals::wheelSpeed;

	};

	static void setup(int  argc, char ** argv);

	// These are the UI Settings
	static std::string lastFilename;
	static std::string lastAnnotationFilename;
	static std::string autoLoadFile;

	static bool showBaseWireframe;
 	static bool showNumbers;
	static bool addGridLines;
	static float xScale_slider;
	static float yScale_slider;
	static float zScale_slider;
	static bool showInfoPanel;
	static int transformType;
	static float   cubeSize;
	static bool showNearest;
	static int numViewAnnotations;
	static bool mergeIdents;
	static int windowWidth;
	static int windowHeight;
	static bool enableRemoteControl;

 	static float hlFilterSize;
	static bool highlightFilter;
	static float detail;
	static bool displaySides;
	static bool flatLighting;
	static bool experimentalMzml;
	static float drawTarget;
	static bool dragMode;
	static bool expertMode;

	// These are used otherwise as globals
	static std::string version;
	static int maxViewAnnotations;

	static ImVec4 clearColour;
	static std::string loadPath;
	static std::string iniFilePath;
	static std::string logFilePath;
	static std::string iniFileName;
	static std::string logFileName;
	static vec3<float> scale;
	static bool disableCamera;
	static void loadSettings();
	static void saveSettings();
	static std::vector<std::string > transformTypes;
	static float nearPlane;
	static float   farPlane;
	static float baseRemoval;
	static bool remoteCamera;
	static float wheelSpeed;
	static float mouseSpeed;
	static int axisMarker;
	
	static std::string lastCameraString;

	static int colourScheme;
	static bool autoCorrelate;
	static float correlateRatio;
	static float correlateOffset;
	static std::string noiseValue;
	static bool noiseRemoval;
	static bool negativeNoiseRemoval;
	static bool colouredGridLines;
	static float peakScale;
	static std::string xLabel;
	static std::string yLabel;
	static std::string xLabels;
	static std::string yLabels;
	static std::string xLabeld;
	static std::string yLabeld;
	static bool preGridLines;
};

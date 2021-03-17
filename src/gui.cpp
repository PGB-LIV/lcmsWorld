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

#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
#define _WIN32
#endif 

#ifdef _WIN32
#include <Windows.h>
#endif

bool fileOpen(std::string filePathName);
bool reloadFile();

void restart();

const float gui::statusHeight = 35;
TimeStamp gui::lastMenuTime = { 0 };

const std::string colOptions[] = { "Default","Alternative" };


void gui::scrollWheel(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}
#include "../files/OpenSans.h"
#include "../files/OpenSans_Italic.h"

#include "../files/OpenSans_Bold.h"

std::string aboutText[] = { "lcmsWorld","  " CUR_VERSION_STRING ,"","This version is for testing, it is not guaranteed to perform correctly","","(c) University of Liverpool 2021","",
"For any issues, please go to the github page",
"github.com/PGB-LIV/lcmsWorld"
};

void gui::setup(const char* glsl_version)
{
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	//	ImGui::GetStyle().ScaleAllSizes(2.0f);
		//	ImGuiStyle::ScaleAllSizes();
 

	ImGuiIO& io = ImGui::GetIO(); 
	(void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

	// ImGui_ImplGlfw_InitForOpenGL sets up some glfw callbacks - window must be focused or this fails
	// symptom is no input - mouse cursor appears but can't select anything

	while (!glfwGetWindowAttrib(Globals::window, GLFW_FOCUSED))
	{
		// window has input focus
		glfwFocusWindow(Globals::window);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ImGui_ImplGlfw_InitForOpenGL(Globals::window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);


	// Setup style
	// ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'misc/fonts/README.txt' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
 
 //   io.Fonts->AddFontFromFileTTF("files/fonts/OpenSans.ttf", 16.0f);

 
// I make a copy because imGui will try to free it later

	void* fontCopy = malloc(sizeof(OpenSans));
	memcpy(fontCopy, OpenSans, sizeof(OpenSans));

	io.Fonts->AddFontFromMemoryTTF(fontCopy, sizeof(OpenSans), 22.0f,NULL, io.Fonts->GetGlyphRangesChineseFull());
	

	fontCopy = malloc(sizeof(OpenSans_Italic));
	memcpy(fontCopy, OpenSans_Italic, sizeof(OpenSans_Italic));

	gui::italic = io.Fonts->AddFontFromMemoryTTF(fontCopy, sizeof(OpenSans_Italic), 22.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());


	fontCopy = malloc(sizeof(OpenSans_Bold));
	memcpy(fontCopy, OpenSans_Bold, sizeof(OpenSans_Bold));

	gui::bold = io.Fonts->AddFontFromMemoryTTF(fontCopy, sizeof(OpenSans_Bold), 22.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());


}
int gui::infoWidth = 240;
ImFont* gui::italic;
ImFont* gui::bold;
// const char * identFilters = ".csv\0\0";
const char * identFilters = ".csv\0.txt\0.mzid\0.mzid.gz\0.mztab\0\0";

const char * lcmsFilters = ".lcms\0.mzml\0.raw\0\0";
 
void gui::setSliderValues()
{

	Settings::zScale_slider = std::max(Settings::zScale_slider, -10.0f);
	Settings::zScale_slider = std::min(Settings::zScale_slider, 10.0f);
	float xScale = Settings::xScale_slider;
	float yScale = Settings::yScale_slider;
	float zScale = Settings::zScale_slider;

	if (Settings::remoteCamera == false)
	{
		Settings::scale.x = xScale < 0 ? -1 / (xScale - 1) : xScale + 1;

		Settings::scale.y = yScale < 0 ? -1 / (yScale - 1) : yScale + 1;

		Settings::scale.z = zScale < 0 ? -1 / (zScale - 1) : zScale + 1;
		Settings::scale.z = std::pow(Settings::scale.z, 1.55f);

	}

}


void gui::MouseMenu()
{
	if (ImGui::SliderFloat("Mouse sensitivity", &Settings::mouseSpeed, 1, 100.0f, "%.0f"))
		Settings::setMouse();

	if (ImGui::SliderFloat("Wheel sensitivity", &Settings::wheelSpeed, 1, 100.0f, "%.0f"))
		Settings::setMouse();

}


void gui::SlidersMenu()
{
	float transformWidth = 230;
	bool rebuild = false;
	if (ImGui::SliderFloat("m/z scale", &Settings::xScale_slider, -10.0f, 10.0f, "%.1f"))
		rebuild = true;


	if (ImGui::SliderFloat("lc scale", &Settings::yScale_slider, -10.0f, 10.0f, "%.1f"))
		rebuild = true;

	if (ImGui::SliderFloat("intensity scale", &Settings::zScale_slider, -10.0f, 10.0f, "%.1f"))
		rebuild = true;

	setSliderValues();
	ImGui::PushItemWidth(transformWidth);

	if (ImGui::BeginCombo("Transform Intensity", Settings::transformTypes[Settings::transformType].c_str()))
	{
		int numTransformOptions = Settings::transformTypes.size();

		for (int i = 0; i < numTransformOptions; i++)
		{
			bool is_selected = (i == Settings::transformType);

			if (ImGui::Selectable(Settings::transformTypes[i].c_str(), is_selected))
			{

				Settings::transformType = i;
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();

		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();
	return;


}

bool gui::numbersBox()
{
	auto buttonText = "Show Grid Lines";
	if (ImGui::Checkbox(buttonText, &Settings::addGridLines))
	{
	}
	buttonText = "Axis Numbering";
	if (ImGui::Checkbox(buttonText, &Settings::showNumbers))
	{
	}
	bool ret = false;
	buttonText = "Show Information Panel";
	if (ImGui::Checkbox(buttonText, &Settings::showInfoPanel))
	{
		auto width = Settings::windowWidth;
		if (Settings::showInfoPanel)
		{
			width = std::max(gui::infoWidth, width - gui::infoWidth);
		}
		Globals::windowWidthActive = width;


		ret = true;
	
	}



	buttonText = "Background Colour";
	if (ImGui::ColorEdit3(buttonText, &Settings::clearColour.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip))
	{

	}



	if (ImGui::BeginCombo("Colour Scheme", colOptions[Settings::colourScheme].c_str()))
	{

		for (int i = 0; i < 2; i++)
		{
			bool is_selected = (i == Settings::colourScheme);

			if (ImGui::Selectable(colOptions[i].c_str(), is_selected))
			{

				Settings::colourScheme = i;
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();

		}
		ImGui::EndCombo();
	}


	return ret;

}

void gui::displayError(Error* e)
{

	std::string title = "Error##" + e->getId();
	ImGui::OpenPopup(title.c_str());
 
	
	if (ImGui::BeginPopupModal(title.c_str()))
	{
		ImGui::Text(e->getText().c_str());
		if (ImGui::Button("Close"))
		{
			Error::removeError();
		}
		ImGui::EndPopup();

	}

 }



void gui::displayPopup(std::string title, std::string text)
{

	std::cout << " popup " << title << "  " << text << "\n";

 	ImGui::OpenPopup(title.c_str());


	

}


void gui::annotationSelector(Landscape* l)
{
	static std::vector<Annotation*> useAnnotations;
	ImGui::PushItemWidth(120);
	if (ImGui::SliderInt("##Choose", &Settings::numViewAnnotations, 0, Settings::maxViewAnnotations, ""))
	{

	}
	ImGui::PopItemWidth();
	ImGui::SameLine();
	std::string buttonText = "Make best " + std::to_string(Settings::numViewAnnotations) + " visible";
	if (ImGui::Button(buttonText.c_str()))
	{
		l->setVisible(Settings::numViewAnnotations);

	}
	ImGui::SameLine();
	if (ImGui::Button("Clear all"))
	{
		l->setVisible(0);
	}

	bool changed = false;
	if (ImGui::Checkbox("Merge Same Idents", &Settings::mergeIdents))
	{
		changed = true;
	}



	static char inputText[128];
	auto getAnnotationsByName = l->getAnnotationsByName();

	std::string lastName = ";";
	if (useAnnotations.size() == 0)
	{
		for (auto a : getAnnotationsByName)
		{
			if ((a->text != lastName) || (!Settings::mergeIdents) )
			useAnnotations.push_back(a);
			lastName = a->text;
		}
	}
	if (ImGui::InputText("Filter", inputText, 128))
	{
		changed = true;
	}
	if (changed)
	{
		useAnnotations.clear();
		std::string inputUpper = toupper(inputText);
		std::string intputString(inputText);

		for (auto a : getAnnotationsByName)
		{
			if ((a->text.find(intputString) != std::string::npos)
				|| (startsWith(a->text, inputUpper))
				|| (toupper(a->accession).find(inputUpper) != std::string::npos)
				|| (startsWith(a->accession,inputText)))
			{
				if ((a->text != lastName) || (!Settings::mergeIdents))
					useAnnotations.push_back(a);
				lastName = a->text;
			}
		}
	}

	ImGui::BeginChild("ScrollingRegion", ImVec2(0, Settings::windowHeight - 380), false, 0);

 
	int lines_count = useAnnotations.size();
	float lines_height = ImGui::GetTextLineHeightWithSpacing();


	ImGuiListClipper clipper(lines_count, lines_height);
	for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
	{
		std::string label = useAnnotations[i]->text + "##" + std::to_string(i);
		std::string clabel =   "##" + std::to_string(i);

		
		if (ImGui::Checkbox(clabel.c_str(), &useAnnotations[i]->isVisible))
		{
		

			bool isVisible = useAnnotations[i]->isVisible;
			std::string name = useAnnotations[i]->text;
	 
			//multiple annotations may have same name

			auto setAnnotation = useAnnotations[i];
			l->setVisible(setAnnotation, isVisible, Settings::mergeIdents);

		}
		ImGui::SameLine();
		ImU32 col = ImColor(255, 0, 0, 0);
		ImGui::PushStyleColor(ImGuiCol_Button, col);

 
		if (ImGui::Button(label.c_str()))
		{
			// getView()->getCamera()->

			DataPoint d{ useAnnotations[i]->mz, useAnnotations[i]->lc, 0 };
					l->getCamera()->updateCameraTarget(d, 2, 0, false);
			l->setVisible(useAnnotations[i], true, Settings::mergeIdents);


		}
		ImGui::PopStyleColor();

	}


	clipper.End();

	ImGui::EndChild();
	int max_on = 100;
	if (ImGui::Button("Select All Filtered"))
	{
		for (auto setAnnotation : useAnnotations)
		{
			l->setVisible(setAnnotation, true, Settings::mergeIdents);
			if (max_on-- < 0)
				break;
		}
	}

}

#define MAX_TEXT_SIZE 32

template <typename T>
static void singleFilter(const char* label, char* inputText, Range<T> worldRange, Range<T> &filterRange, int type)
{
	ImGui::PushItemWidth(90);

	float minx = filterRange.min;
 
	float maxx = filterRange.max;
	float usex = minx;
	if (type &1 )
		usex = maxx;
	if (inputText[0] == 0)
	{
		if (type > 1)
			sprintf(inputText, "%.3e", usex);
		else
			sprintf(inputText, "%.4f", usex);
			

	}
	ImGui::Text(label);
	ImGui::SameLine();

	std::string labelId = "##t" + std::string(label);

	if (ImGui::InputText(labelId.c_str(), inputText, MAX_TEXT_SIZE))
	{
		try
		{
			std::string s(inputText);
			std::istringstream os(s);

			float f;
			os >> f;
			
			f = std::min(f, (float) worldRange.max);
			f = std::max(f, (float) worldRange.min);

			if ((type &1) ==0)
				filterRange.min = f;
			else
				filterRange.max = f;

		}
		catch (...)
		{

		}
	}

	ImGui::SameLine();
	labelId = "##s" +std::string(label);

	//make the slider logarithmic for intensity
	float power = 1;
	if (type > 1)
		power = 10;
	

	if (ImGui::SliderFloat(labelId.c_str(), &usex, worldRange.min, worldRange.max, "", power))
	{
	 

		if ((type&1) == 1)
		{
			if (usex < minx)
				usex = minx;
			filterRange.max = usex;
		}
		else
		{
			if (usex > maxx)
				usex = maxx;
			filterRange.min = usex;
		}

		if (type> 1)
			sprintf(inputText, "%.3e", usex);
		else
			sprintf(inputText, "%.4f", usex);
 


	}
	ImGui::PopItemWidth();


}



void gui::highlightFilter(Landscape* l)
{
//	static char minsig[MAX_TEXT_SIZE];

	//setting.min = 50000;
 
	l->worldSignalRange.min = 1;

 
	if (ImGui::Checkbox("Mark annotations", &Settings::highlightFilter))
	{

	}


 
	ImGui::SameLine();
	ImGui::PushItemWidth(90);

	
	if (ImGui::SliderFloat("Radius", &Settings::hlFilterSize, 0, 10, "%.1f"))
	{
		std::thread t1(&Landscape::setMap,getView());
		t1.detach();
	}
	ImGui::PopItemWidth();

}
void gui::filters(Landscape* l)
{
	

	static char minmz[MAX_TEXT_SIZE];
	static char maxmz[MAX_TEXT_SIZE];
	static char minlc[MAX_TEXT_SIZE];
	static char maxlc[MAX_TEXT_SIZE];
	static char minsig[MAX_TEXT_SIZE];


	//todo - tidy this up
	singleFilter("min. m/z", minmz, l->worldMzRange, l->filter.mzRange,0);
	ImGui::SameLine();
	singleFilter("max. m/z",maxmz, l->worldMzRange, l->filter.mzRange, 1);

	singleFilter("min. time",minlc, l->worldLcRange, l->filter.lcRange, 0);
	ImGui::SameLine();
	singleFilter("max. time",maxlc, l->worldLcRange, l->filter.lcRange, 1);

	// using logarithmic slider, so avoid 0 
	l->worldSignalRange.min = 1;
	if (l->filter.signalRange.min < 1)
		l->filter.signalRange.min = 1;

	//2 signals logarithmic (and 0 is min)
	singleFilter("min. signal", minsig, l->worldSignalRange, l->filter.signalRange, 2);



	return;

}


void gui::infoPanel(ImVec2 menuSize)
{
	static TimeStamp lastChange;

	static std::vector<std::string > lastInfoString;

	std::vector<std::string > infoString;
	if (getView() != NULL)
		infoString = getView()->getInfo();

	if (Settings::showInfoPanel == false)
		return;

	if (infoString != lastInfoString)
	{
		if (Globals::currentTime.time - lastChange.time < 3e5)
		{

			infoString = lastInfoString;
		}
		else
			lastChange = Globals::currentTime;
	}



	lastInfoString = infoString;




	int width, height;


	glfwGetFramebufferSize(Globals::window, &width, &height);
	

	ImGui::SetNextWindowPos(ImVec2( (float) (width - infoWidth), 0));
	ImGui::SetNextWindowSize(ImVec2((float)infoWidth, (float)height));
	ImGui::SetNextWindowBgAlpha(1.0f);


	ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImVec4)ImColor(220, 210, 200));

	ImGui::Begin("Information", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::BeginChild("ScrollingRegionInfo", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);

	ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(30, 30, 30));

	for (auto line : infoString)
	{
		int font = 0;
		//for now <i> indicates italics
		//<c> indicates continuation of previous line

		if (startsWith(line,"<i>"))
		{
			line = line.substr(3);

			font = 1;
			ImGui::PushFont(gui::italic);
		}
		if (startsWith(line, "<b>"))
		{
			line = line.substr(3);

			font = 1;
			ImGui::PushFont(gui::bold);
		}
		if (startsWith(line,"<c>"))
		{
			line = line.substr(3);
			ImGui::SameLine();
		}

		if (line.length() == 0)
			ImGui::Separator();
		else
			ImGui::Text(line.c_str());

		if (font)
		{

			ImGui::PopFont();


		}
	}

	ImGui::PopStyleColor();
	ImGui::EndChild();
	ImGui::End();
	ImGui::PopStyleColor();


}
 
int timesRun = 0;
void gui::fileOpenMenu()
{

	if (getView() == NULL)
		if (ImGui::Button("Load LC-MS File"))
		{
			if (timesRun++  ==0)
				ImGuiFileDialog::Instance()->clear(lcmsFilters);
			else
			{
				ImGuiFileDialog::Instance()->clear(NULL);
			}

			openFileDialog = true;
		}

	if (getView() != NULL)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		if (ImGui::Button("Load LC-MS File"))
		{

		}

		ImGui::PopStyleVar();
		ImGui::PopItemFlag();
	}


	if (getView() == NULL)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

		ImGui::Button("Load annotations File");
		ImGui::PopItemFlag();

		ImGui::PopStyleVar();
	}
	else if (ImGui::Button("Load annotations File"))
	{
		openMzTABFileDialog = true;
		ImGuiFileDialog::Instance()->clear(identFilters);
	}


	//this is when loading via double-click



	std::string button = "Load Last File";
	if ((Settings::lastFilename.length() < 2) || getView() != NULL)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

		if (ImGui::Button(button.c_str()))
		{

		}

		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}
	else
	{

		button += " (" + Settings::lastFilename + ")";
		if (ImGui::Button(button.c_str()))
		{
			if (reloadFile())
				ImGui::SetWindowCollapsed(true);

		}
	}

#ifdef _WIN32

	if (getView() != NULL)
		if (ImGui::Button("Close File & Restart"))
		{



			//		resetView();
			glfwSetWindowShouldClose(Globals::window, true);

			char pathtofile[MAX_PATH];

			GetModuleFileName(GetModuleHandle(NULL), pathtofile, sizeof(pathtofile));
			WinExec(pathtofile, SW_SHOW);


		}

	
		if (ImGui::Button("Start New Instance"))
		{

			//		resetView();
			Settings::saveSettings();
			char pathtofile[MAX_PATH];

			GetModuleFileName(GetModuleHandle(NULL), pathtofile, sizeof(pathtofile));
			WinExec(pathtofile, SW_SHOW);


		}

		if (getView() != NULL)
		if (ImGui::Button("Restart Instance"))
		{

			restart();



		}

#endif


		ImGui::Checkbox("Enable Advanced Settings", &Settings::expertMode);
		if (Settings::expertMode)
		ImGui::Checkbox("Use experimental mzml Reader", &Settings::experimentalMzml);


		

	if (getView() == NULL)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		ImGui::Button("Close File");

		ImGui::PopStyleVar();
		ImGui::PopItemFlag();
	}
}



bool gui::openFileDialogMenu(const char* filters)
{

	bool ret = false;
	static std::string filePathName = "";
	static std::string path = "";
	static std::string fileName = "";
	static std::string filter = "";

	ImGui::SetNextWindowBgAlpha(0.90f);
	Settings::disableCamera = true;

	if (ImGuiFileDialog::Instance()->FileDialog("Choose File", filters, ".", ""))
	{
		if (ImGuiFileDialog::Instance()->IsOk == true)
		{
		 
			filePathName = ImGuiFileDialog::Instance()->GetFilepathName();
			path = ImGuiFileDialog::Instance()->GetCurrentPath();
			fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
			filter = ImGuiFileDialog::Instance()->GetCurrentFilter();
		}
		else
		{
	 
			filePathName = "";
			path = "";
			fileName = "";
			filter = "";
		}
		ret = true;



	}




	filePathName = ReplaceAll(filePathName, "\\\\", "\\");

	if (filePathName.size() > 0)
	{
		std::cout << " load :" << filePathName << ":\n";
		Settings::disableCamera = false;

		if (fileOpen(filePathName))
		{
			ImGui::SetWindowCollapsed(true);

		}
		
		
		filePathName = "";
	}

	if (ret)
		Settings::disableCamera = true;

	return ret;
}

bool gui::openFileDialog = false;
bool gui::openMzTABFileDialog = false;


void  gui::viewMenu(glm::mat4 view)
{



	const char* buttonText = "Show Small Values";




	if (getView() == NULL)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

		ImGui::Checkbox(buttonText, &Settings::showBaseWireframe);
		//	ImGui::Checkbox(buttonText2, &Settings::logTransform);


		ImGui::PopStyleVar();
		ImGui::PopItemFlag();


	}
	else
	{
		ImGui::SliderFloat("Amount of detail", &Settings::detail, 1.0f, 10.0f, "%.1f");


		

		


		// it's stored as an int, because I intend to add different positions
		ImGui::Checkbox("Show Axis Marker", (bool*)&Settings::axisMarker);


		






		if (ImGui::Checkbox(buttonText, &Settings::showBaseWireframe))
		{

			if (getView() != NULL)
				getView()->reBuild();
		}
		ImGui::SameLine();

		if (0)
		if (ImGui::Checkbox("Show full bars", &Settings::displaySides))
		{

			if (getView() != NULL)
				getView()->reBuild();
		}


	}

	if (gui::numbersBox())
	{

	}
	if (ImGui::Checkbox("Share Camera Controls", &Settings::enableRemoteControl))
	{

	}
	ImGui::Checkbox("Uniform Lighting", &Settings::flatLighting);
	ImGui::SameLine();
	ImGui::PushItemWidth(100);

	ImGui::SliderFloat("Show reticule", &Settings::drawTarget,0,100,"%.0f");
	ImGui::PopItemWidth();
	if (Settings::expertMode)
	{
		gui::SlidersMenu();
		if (getView() != NULL)
		{
			ImGui::Separator();
			ImGui::Text("Limit Display");
			gui::filters(getView());
		}
	}

	gui::MouseMenu();
}

std::string helpText[] = { "lcmsWorld","","A 3d viewer for mass spectrometry data", "",  "   ","Instructions:",
"Use the File Menu to load an mzml file",
 
"Hold the left mouse button and use the mouse to rotate around the target",
"Hold the right mouse button and use the mouse to move the target around",
"Click on the Rotate or Drag button to switch the functionality of the mouse buttons",
"Use the mouse wheel to zoom", "",
"Alternatively, use keyboard arrows with shift or controlbto move the camera","",
"When you load an mzml or raw  file, a similarly named .lcms file is created", 
"Load this file next time for instant access","","Load a correctly formatted csv, text, or mzIdentML file to add annotations ",
"you can filter annotations by sequence or protein descriptor", "then select the annotations to view labels","    ",
"The annotation box size is entirely arbitrary for viewing purposes, ", "the exact point is marked in green",
"There is an experimental .mztab loader, but this may not work with all formats  ",

};


void gui::helpMenu()
{

	for (auto s : helpText)
	{
		if (s.size() < 2)
			ImGui::Separator();
		else
			ImGui::Text(s.c_str());
	}


}

static TimeStamp takeShot = { 0 };
const std::string forbiddenChars = "?\"<>| ,!':/\\";
static char ClearForbidden(char toCheck)
{
	if (forbiddenChars.find(toCheck) != std::string::npos)
	{
		return '_';
	}

	return toCheck;
}


bool SaveImage( std::string szPathName, const std::vector<unsigned char>& lpBits, int w, int h) {
	// Create a new file for writing
#ifdef _WIN32

	std::ofstream pFile(szPathName, std::ios_base::binary);
	if (!pFile.is_open()) {
		std::cout << "error save " << szPathName << "\n";

		return false;
	}

	BITMAPINFOHEADER bmih;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = w;
	bmih.biHeight = h;
	bmih.biPlanes = 1;
	bmih.biBitCount = 24;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = w * h * 3;

	BITMAPFILEHEADER bmfh;
	int nBitsOffset = sizeof(BITMAPFILEHEADER) + bmih.biSize;
	LONG lImageSize = bmih.biSizeImage;
	LONG lFileSize = nBitsOffset + lImageSize;
	bmfh.bfType = 'B' + ('M' << 8);
	bmfh.bfOffBits = nBitsOffset;
	bmfh.bfSize = lFileSize;
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;

	// Write the bitmap file header
	pFile.write((const char*)&bmfh, sizeof(BITMAPFILEHEADER));
//	UINT nWrittenFileHeaderSize = pFile.tellp();

	// And then the bitmap info header
	pFile.write((const char*)&bmih, sizeof(BITMAPINFOHEADER));
//	UINT nWrittenInfoHeaderSize = pFile.tellp();

	// Finally, write the image data itself
	//-- the data represents our drawing
	pFile.write((char*) &lpBits[0], lpBits.size());
//	UINT nWrittenDIBDataSize = pFile.tellp();
	pFile.close();
#endif
	return true;
}


void saveScreenshotToFile(std::string filename, int windowWidth, int windowHeight) {
	windowWidth &= ~3;
	windowHeight &= ~3;

	const int numberOfPixels = windowWidth * windowHeight * 3;


	unsigned char *pixels = (unsigned char*) std::malloc(numberOfPixels);

	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_FRONT);
 
	glReadPixels(0, 0, windowWidth, windowHeight, GL_BGR, GL_UNSIGNED_BYTE, pixels);

	const std::vector<unsigned char>px(pixels, pixels+numberOfPixels);
	
 
	SaveImage(filename,px, Settings::windowWidth, Settings::windowHeight);

	free(pixels);
	
}

static std::string curAnnotation;
static int annotationShot = -1;
static float lastSignal;
Annotation* curAnnotationP;

float shotDistance;
void nextAnnotation(Landscape *l, bool reset = false)
{
	if (reset)
	{
		annotationShot = -1;
	}
	auto useAnnotations = l->getAnnotationsByName();

	if (annotationShot > -1)
		l->setVisible(useAnnotations[annotationShot], false, false);

	annotationShot++;
	unsigned int i = annotationShot;
	if (i >= useAnnotations.size())
		return;
	if (useAnnotations[i]->text != curAnnotation)
		lastSignal = 0;

	while (useAnnotations[i]->text == curAnnotation)
	{
 		if (useAnnotations[i]->signal > lastSignal)
			break;
		if (i >= useAnnotations.size() - 1)
			return;
		i++;
		annotationShot = i;

	}
 
 

	if (i >= useAnnotations.size())
		return;


	DataPoint d{ useAnnotations[i]->mz, useAnnotations[i]->lc, 0 };

	shotDistance = 125; // +useAnnotations[i]->signal / 5e5;

	l->getCamera()->updateCameraTarget(d, 3, 0, false);
	//	l->getCamera()->updateCameraTarget(d, 2, 0, false);
	if (i > 0)
		l->setVisible(useAnnotations[i-1], false, false);
	useAnnotations[i]->shortText = "\n\n"+ useAnnotations[i]->shortText;
	useAnnotations[i]->shortText = "";
	l->setVisible(useAnnotations[i], true, false);
	curAnnotationP = useAnnotations[i];
	curAnnotation = useAnnotations[i]->text;
	takeShot.time = 3;
	lastSignal = useAnnotations[i]->signal;


}
bool compareDPI(DataPointInfo  a, DataPointInfo  b) {
	if (a.lc != b.lc)
	return (a.lc < b.lc); 
	return (a.mz < b.mz);
}

void checkScreenShot(Landscape *l)
{
	
 	if (takeShot.time > 0)
	{
		
		l->zScale = 5e-7f;

		takeShot.time -= System::frameTime;

		if (takeShot.time <= 0)
		{
			takeShot.time = 0;
	
			std::string annotationText = curAnnotation;

			annotationText.erase(std::remove(annotationText.begin(), annotationText.end(), '\n'), annotationText.end());

			std::transform(annotationText.begin(), annotationText.end(), annotationText.begin(), ClearForbidden);

			if (annotationText.length() > 160)
				annotationText = annotationText.substr(0, 160);

			size_t lastindex = Globals::lastFileName.find(".");
			std::string filename = Globals::lastFileName.substr(0, lastindex) + "_" + annotationText;
			
			filename += ".bmp";
			saveScreenshotToFile(filename, Settings::windowWidth, Settings::windowHeight);

#if 0
			filename += ".3d";

 			std::vector<DataPointInfo> data = l->findDataPoints(curAnnotationP->mz, curAnnotationP->lc, 0);

			std::sort(data.begin(), data.end(), compareDPI);

			std::ofstream datafile;
			datafile.open(filename);

			for (auto dp : data)
			{
				datafile << dp.lc << "," << dp.mz << "," << dp.signal << "," << (dp.signal / l->worldSignalRange.max) << "\n";
			}
			datafile.close();
#endif

			nextAnnotation(l);

		}
	}
}



std::string curlText[] = {
"These notices only apply to the relevant portion of the software, and not to lcmsWorld as a whole.\n\n"
	
"ImGui \n"
"__________\n"
"Copyright (c) 2014-2019 Omar Cornut "
"Permission is hereby granted, free of charge, to any person obtaining a copy "
"of this software and associated documentation files (the 'Software'), to deal "
"in the Software without restriction, including without limitation the rights "
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
"copies of the Software, and to permit persons to whom the Software is "
"furnished to do so, subject to the following conditions: "
" "
"The above copyright notice and this permission notice shall be included in all "
"copies or substantial portions of the Software. \n"
 " \n"
 "RawFileReader reading tool.\n"
	"__________\n"
	"Copyright 2016 by Thermo Fisher Scientific, Inc.\n"
	"All rights reserved.\n"
	"\n"
	"libCurl \n"
	"__________\n"
"Copyright(c) 1996 - 2019, Daniel Stenberg, daniel@haxx.se, and many contributors, see the THANKS file."
"All rights reserved."
"\n"
"Permission to use, copy, modify, and distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies."
"\n\n"
};

void gui::aboutMenu()
{

	for (auto s : aboutText)
	{
		if (s.size() < 2)
			ImGui::Separator();
		else
			ImGui::Text(s.c_str());
	}

	std::string title = "Licenses";
	std::string text = curlText[0];
	if (ImGui::Button("Display third-party licenses"))
		displayPopup(title,text );

	ImGui::SetNextWindowSize(ImVec2(800, 600));

	if (ImGui::BeginPopupModal(title.c_str()))
	{
		ImGui::TextWrapped(text.c_str());


		if (ImGui::Button("Close"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();

	}
}

void gui::annotationsMenu(glm::mat4 view)
{
	ImGui::PushItemWidth(250);

	if (getView() != NULL)
		if (getView()->hasAnnotations())
		{
			std::string fname = Annotations::filename;
 

			while (fname.length() > 60)
			{
				ImGui::Text(fname.substr(0,60).c_str());
				fname = fname.substr(60);
			}
			ImGui::Text(fname.c_str());
			


		}

	ImGui::SliderFloat("Annotation highlight size", &Settings::cubeSize, 1.0f, 500.0f, "%.1f");
	ImGui::PopItemWidth();
	if (ImGui::Checkbox("Show closest annotation", &Settings::showNearest))
	{


	}


	if (getView() != NULL)
		if (getView()->hasAnnotations())
		{
			highlightFilter(getView());

				annotationSelector(getView());
		}
}
void gui::fileMenu(glm::mat4 view)
{

	// Start the Dear ImGui frame



	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.

//	if (std::rand() > RAND_MAX/10)
	if (1)
	{



		ImGui::SetNextWindowPos(ImVec2(0, 0));
		//		ImGui::SetNextWindowSize(ImVec2(500, 600));
		ImGui::SetNextWindowBgAlpha(0.60f);



		fileOpenMenu();


		//		     ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				//ImGui::End();
		if (Settings::expertMode)
		if (getView())
		if (getView()->hasAnnotations())

		
		if (ImGui::Button("Save Screenshots of All Annotations"))
		{
			ImGui::SetWindowCollapsed(true);

			nextAnnotation(getView(), true);
		}

	}



}

void gui::guiLoop()
{
	//no longer used
	assert(false);
}
void gui::guiLoop(glm::mat4 view)
{

	checkScreenShot(getView());

	ImGuiIO& io = ImGui::GetIO(); (void)io;


	glfwPollEvents();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	gui::labels();
 


	Error * e = Error::getNextError();
	if (e != NULL)
	{
		gui::displayError(e);
	}


#if 0  // time-restricted demo
	std::tm timeinfo = std::tm();
	timeinfo.tm_year = 120;   // year: 2020
	timeinfo.tm_mon = 6;      // month: january
	timeinfo.tm_mday = 1;     // day: 1st
	std::time_t tt = std::mktime(&timeinfo);


	auto time = std::chrono::system_clock::now();
	time_t ttn = std::chrono::system_clock::to_time_t(time);

	if (ttn > tt)

	{
		ImGui::Begin("License Expired" ,NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::Text("This demo version is out of date");
		ImGui::Text("and is no longer licensed for use.");
		ImGui::Text(" ");
		ImGui::Text("Please download a new version ");
		ImGui::Text("if one is available. ");
		ImGui::End();
		return;
	}
#endif


#if 1
	bool inMenu = false;
	ImVec2 menuSize;
	ImGui::SetNextWindowSize(ImVec2(500, 50));

	int width = 0;
	if (Settings::showInfoPanel)
		width = infoWidth;

	if (ImGui::BeginMainMenuBar(width))
	{
		menuSize = ImGui::GetWindowSize();
		if (ImGui::BeginMenu("File"))
		{
			fileMenu(view);
			inMenu = true;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			viewMenu(view);
			inMenu = true;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Annotations"))
		{
			annotationsMenu(view);
			inMenu = true;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			helpMenu();
			inMenu = true;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("About"))
		{
			aboutMenu();
			inMenu = true;
			ImGui::EndMenu();
		}


		ImGui::EndMainMenuBar();
	}

	static TimeStamp lastMenuTime;
	if (inMenu)
		lastMenuTime = Globals::currentTime;

	Settings::disableCamera = ((Globals::currentTime.time - lastMenuTime.time) < 5e5);

	if (openMzTABFileDialog)
	{

		if (openFileDialogMenu(identFilters))
			openMzTABFileDialog = false;
	}
	if (openFileDialog)
	{
		if (openFileDialogMenu(lcmsFilters))
			openFileDialog = false;
	}

	infoPanel(menuSize);



#endif


}




void gui::decorations()
{
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	float buttonWidth = 90;

 
	float xSize = io.DisplaySize.x - 5;

	if (Settings::showInfoPanel)
		xSize -= gui::infoWidth;

 
	ImGui::SetNextWindowBgAlpha(.30f);

	ImGui::SetNextWindowSize(ImVec2(xSize, (float)statusHeight));

	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y), 0, ImVec2(-0.0f, 1.0f));

	if (ImGui::Begin("Status", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
	{
		
		

		ImGui::Text(Globals::statusText.c_str());

		if (Cache::queueSize > 9)
		{
			ImGui::SetCursorPos(ImVec2(xSize - buttonWidth - 100, 5.0f));
			ImGui::Text("loading %d", Cache::queueSize);
		}

 
		ImGui::SetCursorPos(ImVec2(xSize- buttonWidth, 1.0f));
		if (Settings::dragMode)
		{
			if (ImGui::Button("Drag", ImVec2(buttonWidth, statusHeight-3)))
			{
				Settings::dragMode = !Settings::dragMode;
			}
		}
		else
		{
			if (ImGui::Button("Rotate", ImVec2(buttonWidth, statusHeight-3)))
			{
				Settings::dragMode = !Settings::dragMode;
			}
		}

		 
		ImGui::End();
	}
	// Rendering


}

void gui::labels()
{


	if (getView() != NULL)
	{

		ImGui::SetNextWindowPos(ImVec2(-100, -100));
		ImGui::SetNextWindowSize(ImVec2(2920, 2380));
		ImGui::SetNextWindowBgAlpha(0.0f);


		//	ImGui::PushItemFlag(ImGuiWindowFlags_NoInputs, true);

		static bool vis = true;
		ImGui::Begin("Labels", &vis, ImGuiWindowFlags_NoInputs);                         
//		ImColor labelCols[] = { ImColor(250, 250, 250, 255) ,ImColor(160, 255, 160, 255) ,ImColor(240, 240, 240, 255) ,ImColor(255, 230, 230, 255) };
		ImColor labelCols[] = { ImColor(250, 250, 250, 255) ,ImColor(230, 255, 60, 195) ,ImColor(255,255,255, 255) ,ImColor(255, 255, 255, 255) };

		for (int i = 0; i < 4; i++)
		{
			auto labels = getView()->getLabels(i);


			for (Label label : labels)
			{
				std::string text = label.text;
				int wd = (gui::infoWidth * Settings::showInfoPanel) + 40;
				if ( (label.x > Settings::windowWidth- wd)
					|| (label.y < gui::statusHeight/2) )
					continue;
	 	 	ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2((float)label.x, (float)label.y), labelCols[i], text.c_str(), 0, 0.0f, 0);
			

			}
		}
		if (getView() != NULL)
			getView()->clearLabels();

		//


		ImGui::End();

		//	ImGui::PopItemFlag();

	}


}



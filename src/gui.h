#pragma once

#include "Error.h"
#include "glm/glm.hpp"
#include "Structs.h"
#include "SystemSetup.h"
class Landscape;


class gui
{
public:
	static const float statusHeight;
	static ImFont* italic;
	static ImFont* bold;
	static void decorations();
	static 	void labels();
	static void setup(const char* glsl_version);
	static void setSliderValues();

	static void scrollWheel(GLFWwindow* window, double xoffset, double yoffset);
	static	void guiLoop(glm::mat4 view);
	static	void guiLoop();

	static int infoWidth;
	static bool inMenu() {return ((Globals::currentTime.time - lastMenuTime.time) < (0.30 * 1e6));	}
	static void highlightFilter(Landscape* l);
private:
	static void displayPopup(std::string title, std::string text);

	static TimeStamp lastMenuTime;

	static void MouseMenu();
	static void SlidersMenu();
	static bool numbersBox();
	static void filters(Landscape* l);
	static void annotationSelector(Landscape* l);
	static void displayError(Error* e);
	static void infoPanel(ImVec2 menuSize);
	static	void fileOpenMenu();
	static	bool openFileDialogMenu(const char* filters);
	static	void viewMenu(glm::mat4 view);
	static	void helpMenu();
	static	void aboutMenu();
	static	void annotationsMenu(glm::mat4 view);
	static	void fileMenu(glm::mat4 view);

	static bool openFileDialog;
	static bool openMzTABFileDialog;

	static inline Landscape* getView() {
		return System::primary;
	}
};
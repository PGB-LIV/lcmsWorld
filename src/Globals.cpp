#include "Globals.h"
#include "Utils.h"
#include "Settings.h"
TimeStamp Globals::currentTime;
 std::chrono::time_point<std::chrono::high_resolution_clock> Globals::startTime;

 bool Globals::firstSet = false;
 long long Globals::loopCount = 0;
 GLFWwindow* Globals::window = NULL;
 bool Globals::closing = false;
 std::string Globals::statusText;
 std::string Globals::lastFileName;
 std::vector<std::string> Globals::script;
 int Globals::windowWidthActive = 1920;

 void Globals::setFileStatus(std::string filename)
 {
	 lastFileName = filename;
#if 0
	 std::string fname = filename.substr(1 + filename.rfind(sep));
	 std::replace(fname.begin(), fname.end(), '_', ' ');
	 statusText = fname.substr(0, fname.find("."));
#endif
	 statusText = filename;

 }
 float Globals::mouseSpeed = 1.0f;
 float Globals::wheelSpeed = 1.0f;
 double Globals::windowScale = 1.0;
  std::array<std::string, 4> Globals::supported_versions = { "0.14","0.24","0.25", CUR_VERSION_STRING };

#include "Globals.h"
#include "Utils.h"
#include "Settings.h"
TimeStamp Globals::currentTime;
 std::chrono::time_point<std::chrono::high_resolution_clock> Globals::startTime;

 bool Globals::firstSet = false;
 long long Globals::loopCount = 0;
 GLFWwindow* Globals::window = NULL;
 std::string Globals::statusText;
 std::string Globals::lastFileName;
 std::vector<std::string> Globals::script;

 void Globals::setFileStatus(std::string filename)
 {
	 lastFileName = filename;
	 std::string fname = filename.substr(1 + filename.rfind(sep));
	 std::replace(fname.begin(), fname.end(), '_', ' ');
	 statusText = fname.substr(0, fname.find("."));

 }
 float Globals::mouseSpeed = 1.0f;
 float Globals::wheelSpeed = 1.0f;

  std::array<std::string, 3> Globals::supported_versions = { "0.14","0.24", CUR_VERSION_STRING };

#pragma warning (disable : 4367)
#include "gl3w/gl3w.h"    // Initialize with gl3wInit()

#include "ImGuiFileDialog.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#if _WIN32
	#include "dirent_portable.h"
#else
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/file.h>         // for flock (file lock)
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#define stricmp strcasecmp

#endif
#include <iostream>

#include <string>
#include "Utils.h"
#include "Render.h"
#define IMGUI_DEFINE_MATH_OPERATORS
 
#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif
#include <algorithm>

 

inline std::vector<std::string> splitStringVector(const std::string& text, char delimiter)
{
	std::vector<std::string> arr;
	std::string::size_type start = 0;
	std::string::size_type end = text.find(delimiter, start);

#if _WIN32
#else
	arr.push_back(base);
#endif

	while (end != std::string::npos)
	{
		std::string token = text.substr(start, end - start);
		if (token != "")
			arr.push_back(token);
		start = end + 1;
		end = text.find(delimiter, start);
	}
 
	if (start < text.length())
	arr.push_back(text.substr(start));
	return arr;
}

inline void AppendToBuffer(char* vBuffer, int vBufferLen, std::string vStr)
{
	std::string st = vStr;
	if (st != "" && st != "\n")
		ReplaceString(st, "\n", "");
	int slen = (int) strlen(vBuffer);
	vBuffer[slen] = '\0';
	std::string str = std::string(vBuffer);
	if (str.size() > 0) str += "\n";
	str += vStr;
	int len = vBufferLen - 1;
	if (len > str.size()) len = (int) str.size();
#ifdef MINGW32
	strncpy_s(vBuffer, vBufferLen, str.c_str(), len);
#else
	strncpy(vBuffer, str.c_str(), len);
#endif
	vBuffer[len] = '\0';
}

inline void ResetBuffer(char* vBuffer)
{
	vBuffer[0] = '\0';
}

char ImGuiFileDialog::FileNameBuffer[MAX_FILE_DIALOG_NAME_BUFFER] = "";
int ImGuiFileDialog::FilterIndex = 0;

ImGuiFileDialog::ImGuiFileDialog()
{
	
}

ImGuiFileDialog::~ImGuiFileDialog()
{

}

//gets sorted afterwards
static bool stringComparator(FileInfoStruct a, FileInfoStruct b)
{
	if (a.type != b.type) 
		return (a.type < b.type);
	return (tolower(a.fileName) < tolower(b.fileName));
	
}

void ImGuiFileDialog::ScanDir(std::string vPath)
{
	struct dirent **files = 0;
	int i = 0;
	int n = 0;

	rtrim(vPath);
	

	if ((vPath == base) || (vPath.length() < 1) )
	{
	
#ifdef _WIN32
		TCHAR Buffer[MAX_PATH+1];
		DWORD dwRet;
		dwRet = GetCurrentDirectory(MAX_PATH, Buffer);
		vPath = std::string(Buffer);
#else
		vPath = getenv("HOME");
#endif

	}


	m_FileList.clear();

	if (m_CurrentPath_Decomposition.size() == 0)
	{
		// get currentPath
		DIR *currentDir = opendir(vPath.c_str());
		if (currentDir == 0) // path not existing
		{

			vPath = base; 
			currentDir = opendir(vPath.c_str());
		}


		if (currentDir != 0)
		{

 
			m_CurrentPath = vPath;
 

			closedir(currentDir);
			m_CurrentPath_Decomposition = splitStringVector(m_CurrentPath, sepc);
		}
		else
		{
			return;
		}
	}
	
	/* Scan files in directory */
	n = (int) scandir(vPath.c_str(), &files, NULL,NULL);
	if (n >= 0)
	{
		for (i = 0; i < n; i++)
		{
			struct dirent *ent = files[i];
 			FileInfoStruct infos;

			infos.fileName = ent->d_name;

			if (startsWith(infos.fileName, "."))
				continue;

			if (infos.fileName != base)
			{
				switch (ent->d_type)
				{
				case DT_REG: infos.type = 'f'; break;
				case DT_DIR: infos.type = 'd'; break;
				case DT_LNK: infos.type = 'l'; break;
				}
			}
			if (infos.type == 'f')
			{
 
				size_t lpt = infos.fileName.find_last_of(".");

 
				if (lpt != std::string::npos)
				{
					infos.ext = infos.fileName.substr(lpt);


					if (infos.ext == ".gz")
					{
						//stupid special case with two dots
						if (endsWith(infos.fileName,".mzid.gz") )
							infos.ext = ".mzid.gz";

						
					}
					
				}
			}
			
			if (infos.fileName != base)
			{
				m_FileList.push_back(infos);
			}
		}

		for (i = 0; i < n; i++)
		{
			free(files[i]);
		}
		free(files);
	}

	std::sort(m_FileList.begin(), m_FileList.end(), stringComparator); 

}

void ImGuiFileDialog::SetCurrentDir(std::string vPath)
{
	DIR *dir = opendir(vPath.c_str());

	if ((dir == 0) && vPath != base)
	{
		// shouldn't be needed...(scandir should take care of it)
		SetCurrentDir(base);

		//vPath = ".";
		// dir = opendir(vPath.c_str());
	}
	if (dir != 0)
	{
 
		m_CurrentPath = vPath;
 
		
		closedir(dir);
		m_CurrentPath_Decomposition = splitStringVector(m_CurrentPath, sepc);
	}
 
}

void ImGuiFileDialog::ComposeNewPath(std::vector<std::string>::iterator vIter)
{

	m_CurrentPath = "";
	while (vIter != m_CurrentPath_Decomposition.begin())
	{
		if (m_CurrentPath.size() > 0)
			m_CurrentPath = *vIter + sep + m_CurrentPath;
		else
			m_CurrentPath = *vIter;
		vIter--;
	}

	if (m_CurrentPath.size() > 0)
		m_CurrentPath = *vIter + sep + m_CurrentPath;
	else
		m_CurrentPath = *vIter + sep;

	
}
 

void ImGuiFileDialog::clear(const char* vFilters)
{
	ResetBuffer(FileNameBuffer);

	m_FileList.clear();
	if (vFilters != NULL)	
		m_CurrentFilterExt = std::string(vFilters);
	// SetCurrentDir(m_CurrentPath);
	ScanDir(m_CurrentPath);
}


void ImGuiFileDialog::setCurrentPath(std::string path)
{

	DIR *dir = opendir(path.c_str());
	if (dir == 0)
	{
		path = base;
		
	}


	m_CurrentPath = path;

	ScanDir(m_CurrentPath);
	


}
std::string ImGuiFileDialog::getCurrentPath()
{
	return m_CurrentPath;
}
#include "Settings.h"
bool ImGuiFileDialog::FileDialog(const char* vName,  const char* vFilters, std::string vPath, std::string vDefaultFileName)
{
	bool res = false;


	IsOk = false;
 
 
 	ImGui::SetNextWindowSize(ImVec2(800,std::min(600.0f, Settings::windowHeight*.85f) ), ImGuiSetCond_FirstUseEver);

	// ImGui::Begin(vName, NULL, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::Begin(vName, NULL, 0);

	// show current path
	bool pathClick = false;

	if (vPath.size() == 0) 
		vPath = base;
 

	if (m_FileList.size() == 0)
	{
		if (vDefaultFileName.size() > 0)
		{
			ResetBuffer(FileNameBuffer);
			AppendToBuffer(FileNameBuffer, MAX_FILE_DIALOG_NAME_BUFFER, vDefaultFileName);
		}
 
		ScanDir(vPath);
	}







#ifdef _WIN32

	char szBuffer[1024];
	::GetLogicalDriveStrings(1024, szBuffer);
	char *pch = szBuffer;

	char curPath[8];
 
 
	curPath[0] = m_CurrentPath[0];
	curPath[1] = m_CurrentPath[1];
	curPath[2] = 0;

	ImGui::PushItemWidth(60);
	if (ImGui::BeginCombo("Change Drive", curPath)) 
	{
		while (*pch)
		{
			char letter[32];
			strcpy(letter, pch);
			letter[2] = 0;

			bool is_selected = (m_CurrentPath[0] == letter[0]);
			if (ImGui::Selectable(letter, is_selected))
			{
 				vPath = std::string(pch);
				
				std::cout << "Change drive to " << vPath << "\n";


				ScanDir(vPath);
				m_CurrentPath = (vPath);
				pathClick = true;
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus(); 
			pch = &pch[strlen(pch) + 1];
		}
		ImGui::EndCombo();
	}

	ImGui::PopItemWidth();

#endif







	bool first = true;
	for (std::vector<std::string>::iterator itPathDecomp = m_CurrentPath_Decomposition.begin();
		itPathDecomp != m_CurrentPath_Decomposition.end(); ++itPathDecomp)
	{
		if (itPathDecomp != m_CurrentPath_Decomposition.begin())
			ImGui::SameLine(); 
		if (ImGui::Button((*itPathDecomp).c_str()))
		{
			ComposeNewPath(itPathDecomp);
			pathClick = true;
			break;

		}
		
		#ifdef _WIN32
		ImGui::SameLine();
		ImGui::Text(sep.c_str());
		#else
		if (first == false)
		{
			ImGui::SameLine();
			ImGui::Text(sep.c_str());
		}
		#endif

		first = false;
	}

	ImVec2 size = ImGui::GetContentRegionMax();
	size.x -= 0;
	size.y -= 175.0f;
	

	ImGui::BeginChild("##FileDialog_FileList", size);
 

	for (std::vector<FileInfoStruct>::iterator it = m_FileList.begin(); it != m_FileList.end(); ++it)
	{
		FileInfoStruct infos = *it;
		bool show = true;
		

		std::string str;
 

		str =  infos.fileName;

		//not sure how to deal with links at the moment
 		if (infos.type == 'l')
		 show = false;
		if (infos.type == 'f' && m_CurrentFilterExt.size() > 0 && stricmp(infos.ext.c_str(), m_CurrentFilterExt.c_str()))
		{
			 
			show = false;
		}
		if (show == true)
		{
 
			if (infos.type != 'f')
				ImGui::Image((void*)(intptr_t)Render::FolderTexture, ImVec2(28, 28),ImVec2(0,1), ImVec2(1, 0));
			else
			{
				if (infos.ext==".csv")
					ImGui::Image((void*)(intptr_t)Render::CsvTexture, ImVec2(25, 25), ImVec2(0, 1), ImVec2(1, 0));
				else
					ImGui::Image((void*)(intptr_t)Render::LcmsTexture, ImVec2(25, 25), ImVec2(0, 1), ImVec2(1, 0));
			}

			ImGui::SameLine();
			if (ImGui::Selectable(str.c_str(), (strcasecmp(infos.fileName.c_str(), m_SelectedFileName.c_str())==0)))
			{
				if (infos.type == 'd')
				{


					if (infos.fileName == "..")
					{
						if (m_CurrentPath_Decomposition.size() > 1)
						{
							std::vector<std::string>::iterator itPathDecomp = m_CurrentPath_Decomposition.end() - 2;
							ComposeNewPath(itPathDecomp);
						}
					}
					else
					{

						if (m_CurrentPath[m_CurrentPath.length()-1] == sepc)
							m_CurrentPath +=  infos.fileName;
						else
							m_CurrentPath += sep + infos.fileName;

					
					}
					pathClick = true;
				}
				else
				{

					m_SelectedFileName = infos.fileName;
					ResetBuffer(FileNameBuffer);
					AppendToBuffer(FileNameBuffer, MAX_FILE_DIALOG_NAME_BUFFER, m_SelectedFileName);
				}
				break;
			}
		}
	}

	// changement de repertoire
	if (pathClick == true)
	{
		ScanDir(m_CurrentPath);
		m_CurrentPath_Decomposition = splitStringVector(m_CurrentPath, sepc);
		if (m_CurrentPath_Decomposition.size() == 2)
			if (m_CurrentPath_Decomposition[1] == "")
				m_CurrentPath_Decomposition.erase(m_CurrentPath_Decomposition.end() - 1);
	}

	ImGui::EndChild();

	ImGui::Text("File Name : ");

	ImGui::SameLine();

	float width = ImGui::GetContentRegionAvailWidth();
	if (vFilters != 0) width -= 120.0f;
	ImGui::PushItemWidth(width);
	ImGui::InputText("##FileName", FileNameBuffer, MAX_FILE_DIALOG_NAME_BUFFER);
	ImGui::PopItemWidth();

	if (vFilters != 0)
	{
		ImGui::SameLine();

		ImGui::PushItemWidth(100.0f);
		bool comboClick = ImGui::Combo("##Filters", &FilterIndex, vFilters) || m_CurrentFilterExt.size() == 0;
		ImGui::PopItemWidth();
		if (comboClick == true)
		{
			int itemIdx = 0;
			const char* p = vFilters;
			while (*p)
			{
				if (FilterIndex == itemIdx)
				{
					m_CurrentFilterExt = std::string(p);
					break;
				}
				p += strlen(p) + 1;
				itemIdx++;
			}
		}
	}
	
	if (ImGui::Button("Cancel"))
	{
		IsOk = false;
		res = true;
	}

	ImGui::SameLine();

	if (ImGui::Button("Ok"))
	{
		IsOk = true;
		res = true;
	}

	ImGui::End();

	if (res == true)
	{
		m_FileList.clear();
	}

	return res;
}

std::string ImGuiFileDialog::GetFilepathName()
{
	return GetCurrentPath() + sep + GetCurrentFileName();
}

std::string ImGuiFileDialog::GetCurrentPath()
{
	return m_CurrentPath;
}

std::string ImGuiFileDialog::GetCurrentFileName()
{
	return std::string(FileNameBuffer);
}

std::string ImGuiFileDialog::GetCurrentFilter()
{
	return m_CurrentFilterExt;
}

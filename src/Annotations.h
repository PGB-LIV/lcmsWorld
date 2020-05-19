#pragma once
#include <string>
#include "Landscape.h"
class Annotations
{
public:
	static void loadTextBased(std::string filename, char delim, Landscape* l);

	static void  loadCSVA(std::string filename,  Landscape *l);
	static void  loadTextA(std::string filename, Landscape *l);

		static void loadMZTab(std::string filename, Landscape *l);
		static void loadText(std::string filename, Landscape *l);

		static void loadCSV(std::string filename, Landscape *l);
private:
	
};


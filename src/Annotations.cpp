#include "Annotations.h"
#include <vector>
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include "SampleLoader.h"
#include "Error.h"

#include "Structs.h"

void Annotations::loadTextBased(std::string filename, char delim, Landscape* l)
{
	std::fstream f;

	filename = convertFilename(filename);
	f.open(filename, std::ios::in);
	std::string   line;
	std::vector<std::string> header;

	std::getline(f, line);
	std::map<std::string, int> hd;


	header = Utils::split(line, delim);
	int pep_col = -1;
	int score_col = -1;
	int mz_col = -1;
	int lc_col = -1;
	int intensity_col = -1;

	int ptm_col = -1;
	int accession_col = -1;
	for (unsigned int i = 0; i < header.size(); i++)
	{
		rtrim(header[i]);
		header[i] = tolower(header[i]);
 
 

		if ((header[i] == "peptide")
			|| (header[i] == "sequence")
			|| (header[i] == "modified sequence")
			)
			pep_col = i;


		if ((header[i] == "-10lgp")
			|| (header[i] == "score"))

			score_col = i;

		if (header[i] == "m/z")
			mz_col = i;
		if ((header[i] == "rt")
			|| (header[i] == "retention time"))
			lc_col = i;
		if (header[i] == "intensity")
			intensity_col = i;

		if (header[i] == "accession")
			accession_col = i;
		if (header[i] == "ptm")
			ptm_col = i;
  
	}


	int numAnnotations = 0;
	if ((mz_col < 0) || (lc_col < 0) || (pep_col < 0))
	{
		new Error(Error::ErrorType::file, "This file does not have the required information\nto support annotations with lcmsWorld.");


		return;
	}

	mzFloat maxLc = 0;

	while (std::getline(f, line))
	{
		std::vector<std::string> cells;
		cells = Utils::split(line, delim);

		if (cells.size() < 3)
			continue;
		try
		{
			auto mz_s = cells[mz_col];
			auto lc_s = cells[lc_col];
			std::string name = cells[pep_col];
			std::string ptm;
			if (ptm_col > -1)
				ptm = cells[ptm_col];

			std::string accession;
			if (accession_col > -1)
				accession = cells[accession_col];

			if (name.length() > 1300)
			{
				std::cout << " too long " << name << "\n";
				continue;
			}

			std::string score_s = "0";
			if (score_col > -1)
				score_s = cells[score_col];

			float score = std::stof(score_s.c_str());

			mzFloat mz = std::stof(mz_s.c_str());
			lcFloat lc = std::stof(lc_s.c_str());

			signalFloat intensity = 0;
			if (intensity_col > -1)
				intensity = std::stof(cells[intensity_col].c_str());;

			std::string text = name;
			if (text.length() > 50)
			{
				int point = text.length() / 2;
				while (std::isalpha(text[point]))
				{
					point++;
				}
				name = text.substr(0, point);
				text = text.substr(0, point) + "\n" + text.substr(point);
			}


			maxLc = std::max(maxLc, lc);
 
			ImVec2 size = ImGui::CalcTextSize(name.c_str());
 

			Annotation a = { mz, lc, intensity, score, text,ptm,accession,"", size.x,size.y, 0 };
			l->addAnnotation(a);
			numAnnotations++;

		}
		catch (...) {
			// probably null ?
		}

	}
	l->setVisible(0);
	lcFloat scale = 1.0 / 60;
	if (maxLc <= 60)
		scale = 1;


	l->setAnnotationsLoaded(scale);

	f.close();
}




void Annotations::loadCSV(std::string filename, Landscape* l)
{
	loadTextBased(filename, ',', l);
}









void Annotations::loadText(std::string filename, Landscape* l)
{
	loadTextBased(filename, '\t', l);

}


void  Annotations::loadCSVA(std::string filename, Landscape* l)
{

	std::thread t1(Annotations::loadCSV, filename, l);

	t1.detach();

}

void  Annotations::loadTextA(std::string filename, Landscape* l)
{

	std::thread t1(Annotations::loadText, filename, l);

	t1.detach();

}

void Annotations::loadMZTab(std::string filename, Landscape* l)
{
	std::fstream f;

	filename = convertFilename(filename);

	f.open(filename, std::ios::in);
	std::string   line;
	std::vector<std::string> header;
	while (std::getline(f, line))
	{
		std::istringstream ss(line);
		std::string substr;
		if (startsWith(line, "PEH\t"))
		{
			while (std::getline(ss, substr, '\t'))
			{
				header.push_back(substr);

			}
			break;
		}
	}
	if (header.size() == 0)
	{
		reportError("Invalid mztab file");
		std::cout << " no PEH header found \n";
		new Error(Error::ErrorType::file, "Invalid mztab file");
		return;
	}

	int name_col = -1;
	int mz_col = -1;
	int lc_col = -1;

	for (unsigned int i = 0; i < header.size(); i++)
	{
		if (header[i] == "sequence")
			name_col = i;
		if (header[i] == "mass_to_charge")
			mz_col = i;
		if (header[i] == "retention_time")
			lc_col = i;
	}

	if (name_col < 0)
	{
		reportError("Invalid mztab file");
		std::cout << " sequence not found in mztab\n"; return;
	}
	if (mz_col < 0)
	{
		reportError("Invalid mztab file");
		std::cout << " mz_col not found in mztab\n"; return;
	}
	if (lc_col < 0)
	{
		reportError("Invalid mztab file");
		std::cout << " lc_col not found in mztab\n"; return;
	}


	if ((name_col < 0)
		|| (mz_col < 0)
		|| (lc_col < 0))
	{
		new Error(Error::ErrorType::file, "This mztab file does not have the required information\nto support annotations with lcmsWorld.");

		return;

	}
	while (std::getline(f, line))
	{
		std::istringstream ss(line);
		std::string substr;
		int i = 0;

		std::string name;
		std::string mz_s;
		std::string lc_s;

		while (getline(ss, substr, '\t'))
		{
			if (i == name_col)
				name = substr;
			if (i == mz_col)
				mz_s = substr;
			if (i == lc_col)
				lc_s = substr;
			i++;
		}
		if ((mz_s.length() == 0) || (name.length() == 0) || (lc_s.length() == 0))
			continue;

		try
		{
			mzFloat mz = std::stof(mz_s.c_str());
			lcFloat lc = std::stof(lc_s.c_str());
			lc = lc / 60;
			ImVec2 size = ImGui::CalcTextSize(name.c_str());

			//todo - add score etc
			Annotation a = { mz, lc, (signalFloat)0.0, 0, name,"","","",size.x,size.y, 0 };
			l->addAnnotation(a);
		}
		catch (...) {
			// probably null ?
		}


	}


	f.close();


}


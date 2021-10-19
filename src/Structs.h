//Remove the size_t to int warnings
//as imgui is full of them
//but beware of file pointers etc. being > 32bit!
#pragma warning (disable : 4267)

#pragma once
#define NOMINMAX


#include <string>
#include <iostream>
#include <algorithm>
#include <vector>


typedef std::int64_t int64;

typedef float mzFloat;
typedef float lcFloat;
typedef float signalFloat;
typedef int meshHandle;
typedef uint8_t byte;

typedef std::ifstream* FileHandle;

//typedef std::numeric_limits<mzFloat>::max() mzFloat_max;

#define mzFloat_max std::numeric_limits<mzFloat>::max() 
#define mzFloat_min std::numeric_limits<mzFloat>::min() 
#define lcFloat_max std::numeric_limits<lcFloat>::max() 
#define lcFloat_min std::numeric_limits<lcFloat>::min() 
#define signalFloat_max std::numeric_limits<signalFloat>::max() 
#define signalFloat_min std::numeric_limits<signalFloat>::min() 

template <typename T>
struct vec3
{
	T x,y,z;
};

template <typename T>
struct vec2
{
	T x,y;
};


//describes an MZ or retention time range

template <typename T>
struct Range
{
	T min;
	T max;
	bool set = false;
};

template <typename T>

bool operator==(const Range<T>& lhs, const Range<T>& rhs)
{
	return ((lhs.min == rhs.min) && (lhs.max == rhs.max));
}


struct TimeStamp
{
	double time;
};

struct SignalMz
{
	std::vector<mzFloat> mz;
	std::vector<signalFloat> signal;
};


struct Quad
{
	int x[4];
	int y[4];
	int z[4];
	float normal = 0;
	float uv[4];
};
struct Label {
	int x = 0;
	int y = 0;
	std::string text;
	int flags = 0;
};


struct Annotation {
	mzFloat mz = 0;
	lcFloat lc = 0;
	signalFloat signal = 0;
	float score = 0;
	std::string text;
	std::string ptm;
	std::string accession;
	std::string shortText;

	float width = 0;
	float height = 0;
	short flags = 0;
	bool isVisible = false;
};

inline bool operator <(const Annotation& lhs, const Annotation& rhs)
{
	if (lhs.mz == rhs.mz)
	{
		return (lhs.lc < rhs.lc);
	}
	return (lhs.mz < rhs.mz);
}
class Tile;

 struct DataPointInfo {
	mzFloat mz;
	lcFloat lc;
	signalFloat signal;
	int LOD;
	Tile* tile;
 


}; 
struct DataPoint {
	mzFloat mz;
	lcFloat lc;
	signalFloat signal;
 
};
//square MZdata has one mz array for each set of data - jagged has one for each scan
enum class  MZDataType {square, jagged};

enum class  DrawStatus {noData, loadingData, noMesh, creatingMesh, noGLMesh, creatingGLMesh , ready , deleting, reCreatingMesh, reCreatingGLMesh, reCreateMesh, reCreateGLMesh
};
enum class  MeshStatus { noData, hasHandle, loadingData,  ready };
enum class  FadeStatus { initial, fading, complete };
enum  transformTypes { linearTransform, sqrtTransform, logTransform};

struct GLDraw;

struct DeferDraw
{
	float alpha;
	GLDraw *drawObject;
};

struct DataSource {
	int sourceIndex;
	 long long offset;
	 long long size;
	 long long compressed_size;
};

struct MZDataInfo
{
	Range<mzFloat> mzRange = { 3e37f,0 };
	Range<lcFloat> lcRange = { 3e37f,0 };
	Range<signalFloat> signalRange = { 3e37f,0 };
	int num_points = 0;
};

inline bool operator==(const MZDataInfo& lhs, const MZDataInfo& rhs)
{
	return ((lhs.mzRange == rhs.mzRange) && (lhs.lcRange == rhs.lcRange));
}

struct AnnotationInfo
{
	mzFloat mz;
	lcFloat lc;
	std::string text;

};

struct worldView
{
	signalFloat peakHeight;
	mzFloat worldSizeMz;
	lcFloat worldSizeLc;
};

static inline bool endsWith(const std::string  &fullString, const std::string  &ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}

static inline bool startsWith(const std::string  &fullString, const std::string  &start) {
	return (fullString.rfind(start, 0) == 0);
}

 
void reportError(std::string message, int severity = 0);

std::string tolower(std::string a);
 
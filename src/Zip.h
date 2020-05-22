#pragma once
 

class Zip
{
public:
	static int GetMaxCompressedLen(int nLenSrc);
	
	static int CompressData(const byte* abSrc, int nLenSrc, byte* abDst, int nLenDst);
 

	static int UncompressGzipData(const byte* abSrc, size_t nLenSrc, byte* abDst, size_t nLenDst);
	 

	static int UncompressData(const byte* abSrc, size_t nLenSrc, byte* abDst, size_t nLenDst);
 
};

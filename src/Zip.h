#pragma once

class Zip
{
public:
	static int GetMaxCompressedLen(int nLenSrc);
	
	static int CompressData(const BYTE* abSrc, int nLenSrc, BYTE* abDst, int nLenDst);
 

	static int UncompressGzipData(const BYTE* abSrc, size_t nLenSrc, BYTE* abDst, size_t nLenDst);
	 

	static int UncompressData(const BYTE* abSrc, size_t nLenSrc, BYTE* abDst, size_t nLenDst);
 
};

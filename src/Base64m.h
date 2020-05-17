#pragma once
 
#include <vector>
#include <string>
#include <string_view>
typedef unsigned char BYTE;

std::string base64_encode_mem(BYTE const* buf, unsigned int bufLen);
std::vector<BYTE> base64_decode_mem(std::string_view const&);
std::vector<BYTE> base64_decode_mem(std::string_view const&, int startPos, int endPos);


class Base64
{
public:
	static int decoded_size;
};

 


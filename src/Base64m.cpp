#include "Base64m.h"

 
#include <iostream>


static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

int Base64::decoded_size;

static inline bool is_base64(BYTE c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode_mem(BYTE const* buf, unsigned int bufLen) {
	std::string ret;
	int i = 0;
	int j = 0;
	BYTE char_array_3[3];
	BYTE char_array_4[4];

	while (bufLen--) {
		char_array_3[i++] = *(buf++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i <4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';
	}

	return ret;
}

BYTE ret_buffer[1024*1024*8];

BYTE charIndex[256];

bool index_set = false;
void set_index()
{
	for (unsigned int i = 0; i < base64_chars.size(); i++)
	{
		charIndex[base64_chars[i]] = i;
	}
	index_set = true;
}

inline std::vector<BYTE> base64_decode_mem(std::string_view const & encoded_string)
{
	std::vector<BYTE> ret2;
	return ret2;

	//return base64_decode_mem(encoded_string, 0, (int) encoded_string.size());


}

//it may be slightly faster to not have to make  a substring - just use sthe start and endPos

std::vector<BYTE> base64_decode_mem(std::string_view const& encoded_string, int startPos, int endPos) {
 

	if (index_set == false)
		set_index();

	int i = 0;
	int j = 0;
	int in_ = startPos;
	BYTE char_array_4[4], char_array_3[3];

	int out_len = 0;

	BYTE* out_ptr = ret_buffer;

	int end = endPos - 3;
 
 

	while (in_ < end)
	{
 
 
		BYTE d0 = charIndex[encoded_string[in_ ]];
		BYTE d1 = charIndex[encoded_string[in_ + 1 ]];
		BYTE d2 = charIndex[encoded_string[in_ + 2 ]];
		BYTE d3 = charIndex[encoded_string[in_ + 3 ]];
	 

		*out_ptr++ = (d0 << 2) + ((d1 & 0x30) >> 4);
		*out_ptr++ = ((d1 & 0xf) << 4) + ((d2 & 0x3c) >> 2);
		*out_ptr++ = ((d2 & 0x3) << 6) + d3;
		in_ += 4;

	}

	i = endPos - in_ ;
	 
	if (i) {

		for (j = 0; j <i; j++)
			char_array_4[j] = charIndex[encoded_string[in_++]];

		for (j = i; j <4; j++)
			char_array_4[j] = 0;


		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
		{
			//	ret.push_back(char_array_3[j]);
			*out_ptr++ = char_array_3[j];

		}
	}

	Base64::decoded_size = in_;

	std::vector<BYTE> ret2(ret_buffer, out_ptr);

	return ret2;
}
 

std::vector<BYTE> base64_decode_mem(char * encoded_string) {
 

	if (index_set == false)
		set_index();

	int i = 0;
	int j = 0;
	int in_ = 0;
 

	int out_len = 0;

	BYTE* out_ptr = ret_buffer;
 

	char * in_p = encoded_string;
	//BYTE* end_p = in_p + in_len - 3;

	while (*in_p != 0)
	{
		BYTE d0 = charIndex[*in_p++];
		BYTE d1 = charIndex[*in_p++];
		BYTE d2 = charIndex[*in_p++];
		BYTE d3 = charIndex[*in_p++];

		/*
		BYTE d0 = index[encoded_string[in_]];
		BYTE d1 = index[encoded_string[in_ + 1]];
		BYTE d2 = index[encoded_string[in_ + 2]];
		BYTE d3 = index[encoded_string[in_ + 3]];
		*/

		*out_ptr++ = (d0 << 2) + ((d1 & 0x30) >> 4);
		*out_ptr++ = ((d1 & 0xf) << 4) + ((d2 & 0x3c) >> 2);
		*out_ptr++ = ((d2 & 0x3) << 6) + d3;
	 

	}

	 

	Base64::decoded_size = in_;

	std::vector<BYTE> ret2(ret_buffer, out_ptr);

	return ret2;
}

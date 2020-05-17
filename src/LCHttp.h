#pragma once
 
#include "Structs.h"
#include <vector>
#include <string>
#include "curlinc/curl/curl.h"
struct MemoryStruct {
	byte *memory;
	size_t size;
};

//typedef CURL;

class LCHttp
{
public:
	static std::vector<byte> getFileFromHttp(std::string url);
	static std::vector<byte> getFromHttp(DataSource);
	static std::string loadUrl;
	static void start();
	static void finish();

private:
	static void delayReset();
	static void delayRetry();
	static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
	static struct MemoryStruct get_response(std::string url);
	static struct MemoryStruct  get_http(long long offset, size_t size);
	static CURL *curl;
};
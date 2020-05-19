
 
#include "LCHttp.h"

// Curl did not build on Linux
// But it's not actively supported yet anyway, so will leave it out for now
#ifdef _WIN32


#include <chrono>
#include <thread>
#include "Zip.h"
#include <mutex>

#include "curlinc/curl/curl.h"

CURL *LCHttp::curl = NULL;
auto min_sleep = std::chrono::milliseconds(1);
auto max_sleep = std::chrono::milliseconds(5000);
auto current_sleep = min_sleep;

//lock is just to prevent closing until finished
static std::mutex lock;

void LCHttp::delayReset()
{
	current_sleep = min_sleep;
}
void LCHttp::delayRetry()
{

	std::this_thread::sleep_for(current_sleep);
	current_sleep *= 2;

	current_sleep = std::min(current_sleep, max_sleep);

}





size_t LCHttp::WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	byte *ptr = (byte*)realloc(mem->memory, mem->size + realsize + 1);
	if (ptr == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

void LCHttp::finish()
{
	lock.lock();
	lock.unlock();
	if (curl != NULL)
	{
		curl_easy_cleanup(curl);
		curl = NULL;
	}
}


void LCHttp::start()
{
	if (curl == NULL)
	{
		curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();

	}
}
struct MemoryStruct LCHttp::get_response(std::string url)
{


	CURLcode res;
	struct MemoryStruct chunk;

	chunk.memory = (byte*)malloc(8);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */



	if (curl) {


		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));


		// curl_easy_cleanup(curl);
	}
	return chunk;
}


struct MemoryStruct  LCHttp::get_http(long long offset, size_t size)
{

	std::string url = loadUrl + "&offset=" + std::to_string(offset);
	url = url + "&size=";
	url = url + std::to_string(size);

	auto	chunk = get_response(url);


	return chunk;
}
 
std::vector<byte> LCHttp::getFileFromHttp(std::string url)
{
	lock.lock();
	auto	chunk = get_response(url);
	std::vector<byte> data(&chunk.memory[0], &chunk.memory[chunk.size]);
	free(chunk.memory);

	lock.unlock();
	return data;
}


std::vector<byte> LCHttp::getFromHttp(DataSource d)
{
	lock.lock();

	 
	std::vector<byte> empty;
	{

		auto chunk = get_http(d.offset, d.compressed_size);

		if (d.compressed_size != d.size)
		{

			if (chunk.size != d.compressed_size)
			{
				delayRetry();
				lock.unlock();

				return empty;
			}

			std::vector<byte> decompressedData(d.size);


			int length = Zip::UncompressData((const BYTE*)chunk.memory, d.compressed_size, &decompressedData[0], decompressedData.size());
			//decompress data 

			free(chunk.memory);


			if (length != d.size)
			{
				std::cout << " decompress size mismatch a " << length << " , " << d.size << "  c " << chunk.size << "  , " << d.compressed_size << "\n";
				delayRetry();
				lock.unlock();

				return empty;
			}

			delayReset();
			lock.unlock();

			return decompressedData;
		}
		else
		{
			std::vector<byte> data(&chunk.memory[0], &chunk.memory[d.size]);
			free(chunk.memory);

			if (chunk.size != d.size)
			{
				std::cout << " size mismatch b" << chunk.size << " , " << d.size << "\n";
				delayRetry();
				lock.unlock();

				return empty;
			}
			delayReset();
			lock.unlock();

			return data;
		}
	}
}


#else

#include "LCHttp.h"















void LCHttp::delayReset()
{

}
void LCHttp::delayRetry()
{

}





size_t LCHttp::WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{

	return 0;
}

void LCHttp::finish()
{
	
}


void LCHttp::start()
{
	
}
struct MemoryStruct LCHttp::get_response(std::string url)
{


	
	struct MemoryStruct chunk;

	return chunk;
}

struct MemoryStruct  LCHttp::get_http(long long offset, size_t size)
{

	struct MemoryStruct chunk;

	return chunk;
}

std::vector<byte> LCHttp::getFromHttp(DataSource d)
{
	return std::vector<byte>();
}
std::vector<byte> LCHttp::getFileFromHttp(std::string url)
{
	return std::vector<byte>();
}


#endif
std::string LCHttp::loadUrl;

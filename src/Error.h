#pragma once
#include "Structs.h"
#include <deque>
#include <chrono>
#include <mutex>
//possibly should be called 'system'
//just to make a few things faster to access from anywhere; e.g., time stamps
//don't want to make function calls every time, once per frame is plenty

class Error
{
public:

 

	static Error* getNextError();
 
	enum class  ErrorType { none, file, misc };
	Error(ErrorType e, std::string text);
	static void removeError();
	std::string getText() { return errorText; }
	std::string getId() { return std::to_string(id); }
private:
	int id;
	static int count;
	static std::mutex queueLock;
	static std::deque<Error*> errors;
	std::string errorText;
	const static std::string  errorTypes[];
	ErrorType type;
	int displayed;
 




};

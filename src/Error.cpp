#include "Error.h"

const std::string  Error::errorTypes[] = { "", "File Error", "Unknown Error" };

std::mutex Error::queueLock;
std::deque<Error*> Error::errors;
  Error* Error::getNextError()
{
	  std::lock_guard<std::mutex> lock(queueLock);
	  Error* e = NULL;
	if (errors.size() > 0)
	{
		e = errors.front();

	}
 

	return e;
}
 
  
    int Error::count = 0;
  void Error::removeError()
  {
	  std::lock_guard<std::mutex> lock(queueLock);

	  Error* e = errors.front();
	  errors.pop_front();
	  delete e;
 
	  
  }

Error::Error(ErrorType e, std::string text)
{
	type = e;
	errorText = text;
	displayed = 0;
 
 
	std::lock_guard<std::mutex> lock(queueLock);


	if (errors.size() > 0)
	{
		//ignore consecutive errors
		Error *e = errors.back();
		if (text == e->errorText)
			return;
	}
	errors.push_back(this);
 
	id = count++;

}
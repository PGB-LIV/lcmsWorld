#include "Error.h"

const std::string  Error::errorTypes[] = { "", "File Error", "Unknown Error" };

std::mutex Error::queueLock;
std::deque<Error*> Error::errors;
  Error* Error::getNextError()
{
	queueLock.lock();
	Error* e = NULL;
	if (errors.size() > 0)
	{
		e = errors.front();

	}
	queueLock.unlock();

	return e;
}
 
  
    int Error::count = 0;
  void Error::removeError()
  {
	  queueLock.lock();
	  Error* e = errors.front();
	  errors.pop_front();
	  delete e;
	  queueLock.unlock();

	  
  }

Error::Error(ErrorType e, std::string text)
{
	type = e;
	errorText = text;
	displayed = 0;

	queueLock.lock();
	errors.push_back(this);
	queueLock.unlock();
	id = count++;

}
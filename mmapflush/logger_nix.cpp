#include <ctime>
#include <pthread.h>

#include "logger.h"

logstream logger() {
	return logstream();
}

ostringstream& logstream::_logger() {
	if (_os) {
		return *_os;
	}

	_os = new ostringstream();
	_dirty = false;

	time_t now = time(0);
	struct tm ts;
	char timeBuffer[100] = "";
	localtime_r(&now, &ts);
	strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d.%X", &ts);
	*_os << "[" << timeBuffer << " {thread-" << pthread_self() << "}] ";
	return *_os;
}

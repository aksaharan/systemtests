#include <Windows.h>
//#include <WinBase.h>

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
	localtime_s(&ts, &now);
	strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d.%X", &ts);
	*_os << "[" << timeBuffer << " {thread-" << GetCurrentThreadId() << "}] ";
	return *_os;
}
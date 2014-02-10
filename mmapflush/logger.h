#pragma once

#include <Windows.h>
#include <WinBase.h>

#include <iostream>
#include <sstream>
#include <ctime>

using namespace std;

class logstream {
public:
	logstream()
		: _os(NULL), _dirty(false) {
	}

	logstream(const logstream& ls)
		: _os(NULL), _dirty(false) {
	}

	virtual ~logstream() {
		if (_dirty) {
			cout << _os->str();
		}

		if (_os) {
			delete _os;
		}
	}

	template<typename X>
	inline logstream& operator<<(X v) {
		_dirty = true;  _logger() << v;
		return *this;
	}

	inline logstream& operator<<(std::ostream& (*manip)(std::ostream&)) {
		_dirty = true; _logger() << manip;
		return *this;
	}

	inline logstream& operator<<(std::ios_base& (*manip)(std::ios_base&)) {
		_dirty = true; _logger() << manip;
		return *this;
	}

private:
	bool _dirty;
	ostringstream* _os;

	inline ostringstream& _logger() {
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
};

logstream logger();

#pragma once

#include <iostream>
#include <sstream>
#include <ctime>

using namespace std;

class logstream {
public:
	inline logstream()
		: _os(NULL), _dirty(false) {
	}

	inline logstream(const logstream& ls)
		: _os(NULL), _dirty(false) {
	}

	virtual inline ~logstream() {
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

	ostringstream& _logger();
};

logstream logger();

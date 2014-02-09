#pragma once

#include <Windows.h>
#include <WinBase.h>

#include <iostream>
#include <sstream>
#include <map>
#include <string>

using namespace std;

class logstream : public ostringstream {
};

ostream& logger(ostringstream& os = ostringstream());

class MMapFile {
public:
	MMapFile(string filename, void* view, unsigned long long length, HANDLE handle, HANDLE mapHandle)
		: _filename(filename), _view(view), _length(length), _handle(handle), _mapHandle(mapHandle),
		_flushBlockSize(10 * 1024 * 1024) {
	}

	virtual ~MMapFile();

	bool open();
	bool flush(unsigned threads);
	void close();

	void* view() { return _view; } ;
	const unsigned long long& length() const { return _length; }
	const string& filename() const { return _filename; }

	bool _flushView(unsigned long long offset, unsigned long long len);

private:
	string _filename;
	void* _view;
	unsigned long long _length;
	unsigned long long _flushBlockSize;
	HANDLE _handle;
	HANDLE _mapHandle;

	MMapFile(const MMapFile&);
	MMapFile& operator=(const MMapFile&);

	static DWORD _flushThreadedView(LPVOID data);
};

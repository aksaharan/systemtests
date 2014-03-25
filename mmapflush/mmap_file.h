#pragma once

#include <Windows.h>
#include <WinBase.h>

#include <iostream>
#include <sstream>
#include <map>
#include <string>

using namespace std;

namespace {
	const unsigned long long DEFAULT_FLUSH_BLOCK_SIZE = 10 * 16 * 1024;
}


class MMapFile {
public:
	MMapFile(string filename, void* view, unsigned long long length, HANDLE handle, HANDLE mapHandle)
		: _filename(filename), _view(view), _length(length), _handle(handle), _mapHandle(mapHandle),
		_flushBlockSize(DEFAULT_FLUSH_BLOCK_SIZE), _adjBlockFlush(true) {
	}

	virtual ~MMapFile();

	bool open();
	bool flush(unsigned threads);
	void close();

	void* view() { return _view; } ;
	const unsigned long long& length() const { return _length; }
	const string& filename() const { return _filename; }

	void setFlushBlockSize(unsigned long long value) { _flushBlockSize = value <= 0 ? DEFAULT_FLUSH_BLOCK_SIZE : value; }
	void setAdjBlockFlush(bool value) { _adjBlockFlush = value; }

	bool _flushView(unsigned long long offset, unsigned long long len);

private:
	string _filename;
	void* _view;
	unsigned long long _length;
	unsigned long long _flushBlockSize;
	bool _adjBlockFlush;
	HANDLE _handle;
	HANDLE _mapHandle;

	MMapFile(const MMapFile&);
	MMapFile& operator=(const MMapFile&);

	static DWORD _flushThreadedView(LPVOID data);
};

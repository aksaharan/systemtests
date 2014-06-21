#pragma once

#include "mmap_file.h"

using namespace std;

namespace mmapflush {

class MMapFileNix : public MMapFile {
public:
	MMapFileNix(string filename, void* view, unsigned long long length, int fd)
		: MMapFile(filename, view, length), _fd(fd) {
	}

	virtual ~MMapFileNix();

	bool flush(unsigned threads);
	void close();

	bool _flushView(unsigned long long offset, unsigned long long len);

protected:
	int _fd;

	MMapFileNix(const MMapFile&);
	MMapFileNix& operator=(const MMapFileNix&);

	virtual bool _flushView(unsigned long long offset, unsigned long long len) = 0;
};

}

#pragma once

#include <Windows.h>
#include <map>
#include <string>
#include <vector>

using namespace std;

class MMapFile;

class MMapFileManager {
public:
    static void* getNextMemoryMappedFileLocation(unsigned long long mmfSize);
	static size_t fetchMinOSPageSizeBytes();

	static MMapFileManager& getInstance();

	void setFileCreateOptions(DWORD options) { _fileCreateOptions = options; }
	void setZeroFill(bool flag) { _zeroFill = flag; }
	void setFlushThreads(unsigned count) { _flushThreads = count; }
	void setParallelFlush(bool flag) { _parallelFlush = flag; }
	void setRepeatCount(long count) { _repeatCount = count; }
	void setUpdateOffset(long value) { _updateOffset = value; }
	void setUpdateValue(long value) { _updateValue = value; }
	void setFileSize(unsigned long long value) { _fileSize = value; }

	bool runTests(const vector<string>& files);
	MMapFile* mappedFile(const string& filename);

private:
	DWORD _fileCreateOptions;
	bool _zeroFill;
	unsigned _flushThreads;
	bool _parallelFlush;
	long _repeatCount;
	long _updateOffset;
	long _updateValue;
	unsigned long long _fileSize;
	std::map<string, MMapFile*> _mappedFiles;

	MMapFileManager()
		: _fileCreateOptions(FILE_ATTRIBUTE_NORMAL), _zeroFill(false), _flushThreads(1), _parallelFlush(false),
		_repeatCount(1), _updateOffset(70 * 1024), _updateValue(rand()), _fileSize(512LL * 1024 * 1024) {
	}

	~MMapFileManager() {
		_closeAll();
	}

	MMapFileManager(const MMapFileManager&);
	MMapFileManager& operator=(const MMapFileManager);

	HANDLE _createFile(const string& filename, unsigned long long& length);
	unsigned long long _getFileSize(HANDLE handle);

	MMapFile* _map(const string& filename, unsigned long long& length);

	unsigned int _openFiles(const vector<string>& files);
	bool _closeAll();
	bool _flushAll();

	bool _flushFile(MMapFile* mapFile);
	static DWORD _flushFileThreaded(LPVOID data);
};
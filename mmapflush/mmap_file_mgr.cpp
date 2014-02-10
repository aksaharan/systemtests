#include <time.h>

#include "logger.h"
#include "mmap_file.h"
#include "mmap_file_mgr.h"

using namespace std;

// Synchronize in case of multi-threaded invocation
static unsigned long long _nextMemoryMappedFileLocation = 256LL * 1024LL * 1024LL * 1024LL;

void* MMapFileManager::getNextMemoryMappedFileLocation(unsigned long long mmfSize) {
	if (4 == sizeof(void*)) {
		logger() << "sizeof(void*) = " << sizeof(void*) << ", returning 0" << endl;
		return 0;
	}

	//TODO: Scope Lock for multi-threaded
	static unsigned long long granularity = 0;
	if ( 0 == granularity ) {
		SYSTEM_INFO systemInfo;
		GetSystemInfo( &systemInfo );
		granularity = static_cast<unsigned long long>( systemInfo.dwAllocationGranularity );
		logger() << "System[Granularity] = " << reinterpret_cast<void*>(granularity) << endl;
	}
	unsigned long long thisMemoryMappedFileLocation = _nextMemoryMappedFileLocation;
	mmfSize = (mmfSize + granularity - 1) & ~(granularity - 1);
	_nextMemoryMappedFileLocation += mmfSize;
	return reinterpret_cast<void*>(static_cast<uintptr_t>(thisMemoryMappedFileLocation));
}

MMapFileManager& MMapFileManager::getInstance() {
	static MMapFileManager _instance;
	return _instance;
}

size_t MMapFileManager::fetchMinOSPageSizeBytes() {
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	size_t minOSPageSizeBytes = si.dwPageSize;
	if (minOSPageSizeBytes <= 0 || minOSPageSizeBytes >= 1000000) {
		logger() << "MinOS Page size beyond expected ranges {pageSize: " << minOSPageSizeBytes << "}" << endl;
		minOSPageSizeBytes = 0;
	} else if ((minOSPageSizeBytes & (minOSPageSizeBytes - 1)) != 0) {
		logger() << "MinOS Page size beyond expected ranges {pageSize: " << minOSPageSizeBytes << "}" << endl;
		minOSPageSizeBytes = 0;
	}
	return minOSPageSizeBytes;
}

MMapFile* MMapFileManager::mappedFile(const string& filename) {
	std::map<string, MMapFile*>::iterator pIt = _mappedFiles.find(filename);
	if (pIt == _mappedFiles.end()) {
		return NULL;
	}
	return pIt->second;
}

bool MMapFileManager::runTests(const vector<string>& files) {
	// Map all the files to memory
	vector<string> mappedFiles;
	for (vector<string>::const_iterator pIt = files.begin(); pIt != files.end(); ++pIt) {
		if (!_map(*pIt, _fileSize)) {
			logger() << "Failed to create / open memory map file {file: " << *pIt << "}" << endl;
		} else {
			mappedFiles.push_back(*pIt);
		}
	}

	unsigned repeatCount = 1;
	for (unsigned i = 0; i < repeatCount; ++i) {
		for (vector<string>::iterator pIt = mappedFiles.begin(); pIt != mappedFiles.end(); ++pIt) {
			logger() << "Starting loop for dirtying mapped memory region" << endl;
			MMapFile* mapFile = mappedFile(*pIt);
			if (!mapFile) {
				logger() << "Failed to get the mapped file for specified file {file: " << *pIt << "}" << endl;
				continue;
			}

			// Serialized touching of pages 
			char* mappedView = reinterpret_cast<char*>(mapFile->view());
			for (unsigned long long i = 0; i < (mapFile->length() - sizeof(_updateValue)); i += _updateOffset) {
				*((long*)&mappedView[i]) = _updateValue;
			}

			/*
			// Randomized touching of pages 
			char* mappedView = reinterpret_cast<char*>(mapFile->view());
			unsigned long long maxLength = mapFile->length() - sizeof(touchValue);
			char* updatePtr;
			for (unsigned long long i = 0; i < (mapFile->length() - sizeof(touchValue)); i += touchOffset) {
				updatePtr = mappedView + (rand() % maxLength);
				*((long*)updatePtr) = touchValue;
			}
			*/
		}

		if (i < (repeatCount - 1)) {
			// Do not flush on last instance since it will be flushed by closeAll
			_flushAll();
		}
	}

	_closeAll();
	return true;
}

MMapFile* MMapFileManager::_map(const string& filename, unsigned long long& length) {
	// Create / Open Specified file with default set options for the file
	HANDLE handle = _createFile(filename, length);
	if (!handle) {
		logger() << "Failed to create file {file: " << filename << ", length: " << length << "}" << endl;
		return NULL;
	}

	// Create mmap file handle
	HANDLE mapHandle = CreateFileMapping(handle, NULL, PAGE_READWRITE, length >> 32, (unsigned)length, NULL);
	if (!mapHandle) {
		logger() << "Failed to create file mapping {file: " << filename << ", length: " << length << "}" << endl;
		CloseHandle(handle);
		return NULL;
	}

	void* nextAddress = getNextMemoryMappedFileLocation(length);
	void* view = MapViewOfFileEx(mapHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0, (LPVOID)nextAddress);
	if (!view) {
		logger() << "Failed to map file {file: " << filename << ", length: " << length
			<< ", address: " << nextAddress << "}" << endl;
		CloseHandle(mapHandle);
		CloseHandle(handle);
	}

	MMapFile* mapFile = new MMapFile(filename, view, length, handle, mapHandle);
	_mappedFiles.insert(make_pair(filename, mapFile));
	return mapFile;
}

bool MMapFileManager::_closeAll() {
	logger() << "Flushing files before closing {files: " << _mappedFiles.size() << "}" << endl;

	bool status = _flushAll();
	for (std::map<string, MMapFile*>::iterator pIt = _mappedFiles.begin();
			pIt != _mappedFiles.end();
			++pIt) {
		pIt->second->close();

		delete pIt->second;
		pIt->second = NULL;
	}
	_mappedFiles.clear();
	return status;
}

bool MMapFileManager::_flushAll() {
	if (_mappedFiles.size() <= 0) {
		return true;
	}

	bool status = true;

	DWORD* threadIds = NULL;
	HANDLE* threadHandles = NULL;
	if (_parallelFlush) {
		threadIds = new DWORD[_mappedFiles.size()];
		threadHandles = new HANDLE[_mappedFiles.size()];
	}

	logger() << "=== Starting to flush all files now {files: " << _mappedFiles.size() << "}" << endl;
	ULONGLONG startTime = GetTickCount64();

	unsigned i = 0;
	for (std::map<string, MMapFile*>::iterator pIt = _mappedFiles.begin();
		pIt != _mappedFiles.end();
		++pIt, ++i) {
			if (_parallelFlush) {
				threadHandles[i] = CreateThread(NULL /* default security attributes */, 0 /* default stack size */,
					&MMapFileManager::_flushFileThreaded, pIt->second, 0 /* default flags */, &threadIds[i] /* threadId return */);
				if (threadHandles[i] == NULL) {
					logger() << "FATAL: Failed to create thread {i: " << i << "}" << endl;
					status = false;
				}
			} else {
				status = status && _flushFile(pIt->second);
			}
	}

	if (_parallelFlush) {
		WaitForMultipleObjects(static_cast<DWORD>(_mappedFiles.size()), threadHandles, TRUE, INFINITE);
		for (unsigned i = 0; i < _mappedFiles.size(); ++i) {
			CloseHandle(threadHandles[i]);
		}

		delete[] threadIds;
		delete[] threadHandles;
	}

	ULONGLONG flushTime = GetTickCount64();
	logger() << "=== Completed flushing all files {files: " << _mappedFiles.size() 
		<< ", flush-time(millis): " << (flushTime - startTime) << "}" << endl;

	return status;
}

bool MMapFileManager::_flushFile(MMapFile* mapFile) {
	logger() << "Flushing file {file: " << mapFile->filename() << ", length: "
		<< mapFile->length() << "}" << endl;

	ULONGLONG startTime = GetTickCount64();
	bool flushStatus = mapFile->flush(_flushThreads);
	ULONGLONG flushTime = GetTickCount64();

	logger() << "Flushed file {file: " << mapFile->filename() << ", length: " << mapFile->length()
		<< ", flushed-success: " << flushStatus
		<< ", flush-times(millis): " << (flushTime - startTime) << "}" << endl;
	return flushStatus;
}

DWORD MMapFileManager::_flushFileThreaded(LPVOID data) {
	getInstance()._flushFile(reinterpret_cast<MMapFile*>(data));
	return 0;
}

HANDLE MMapFileManager::_createFile(const string& filename, unsigned long long& length) {
	DWORD rw = GENERIC_READ | GENERIC_WRITE;
	HANDLE handle = CreateFile(
		(LPCTSTR)filename.c_str(),
		(GENERIC_READ | GENERIC_WRITE), // desired access
		FILE_SHARE_WRITE | FILE_SHARE_READ, // share mode
		NULL, // security
		OPEN_ALWAYS, // create disposition
		_fileCreateOptions, // flags
		NULL); // hTempl
	if (handle == INVALID_HANDLE_VALUE) {
		DWORD dosError = GetLastError();
		logger() << "CreateFile faield {file: " << filename << ", error: " << GetLastError()
			<< endl;
		return 0;
	}

	bool isNewFile = false;
	DWORD retCode = GetLastError();
	if (retCode == ERROR_ALREADY_EXISTS) {
		// Change the length to correct length
		unsigned long long newLength = _getFileSize(handle);
		logger() << "Changing the incoming length to length of existing file {old: " << length
			<< ", new: " << newLength << "}" << endl;
		length = newLength;
		isNewFile = true;
	}

	return handle;
}

unsigned long long MMapFileManager::_getFileSize(HANDLE handle) {
	LARGE_INTEGER lp;
	if (!GetFileSizeEx(handle, &lp)) {
		return 0;
	}

	unsigned long long l = lp.HighPart;
	l = l << 32 | lp.LowPart;
	return l;
}
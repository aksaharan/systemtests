#include <ctime>
#include <sstream>

#include "mmap_file.h"

ostream& logger(ostringstream& os) {
	// The logger is not thread-safe and may mingle output lines
	time_t now = time(0);
	struct tm ts;
	char timeBuffer[100] = "";
	localtime_s(&ts, &now);
	strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d.%X", &ts);
	return cout << "[" << timeBuffer << " {thread-" << GetCurrentThreadId() << "}] " << os.str();
}

namespace {
	struct ThreadedFlushData {
		MMapFile* _mapFile;
		unsigned long long _blockSize;
		unsigned long long _startOffset;
		unsigned long long _endOffset;

		ThreadedFlushData()
			: _mapFile(NULL), _blockSize(0), _startOffset(0), _endOffset(0) {
		}
	};

	DWORD WINAPI ThreadedDataFlusher(LPVOID data) {
		ThreadedFlushData* mapData = reinterpret_cast<ThreadedFlushData*>(data);
		logger() << "Called Threaded Data Flusher {mapFile: " << mapData->_mapFile->filename() 
			<< ", length: " << mapData->_mapFile->length() << ", startOffset: " << mapData->_startOffset
			<< ", blockSize: " << mapData->_blockSize << ", endOffset: " << mapData->_endOffset
			<< "}" << endl;
		unsigned long long blocksFlushed = 0;
		for (unsigned long long i = mapData->_startOffset; 
			i < mapData->_endOffset; 
			i += mapData->_blockSize, ++blocksFlushed) {
				mapData->_mapFile->_flushView(i, mapData->_blockSize);
				/*
				logger() << "Flushed {current: " <<i << ", startOffset: " << mapData->_startOffset
					<< ", blockSize: " << mapData->_blockSize << ", endOffset: " << mapData->_endOffset
					<< ", flushedBlocks: " << blocksFlushed
					<< "}" << endl;
				*/
		}

		logger() << "Ending Threaded Data Flusher {FlushedBlocks: " << blocksFlushed << "}" << endl;
		return 0;
	}
}

MMapFile::~MMapFile() {
	close();
}

DWORD MMapFile::_flushThreadedView(LPVOID data) {
	ThreadedFlushData* mapData = reinterpret_cast<ThreadedFlushData*>(data);
	logger() << "Called Threaded Data Flusher {mapFile: " << mapData->_mapFile->filename() 
		<< ", length: " << mapData->_mapFile->length() << ", startOffset: " << mapData->_startOffset
		<< ", blockSize: " << mapData->_blockSize << ", endOffset: " << mapData->_endOffset
		<< "}" << endl;
	unsigned long long blocksFlushed = 0;
	for (unsigned long long i = mapData->_startOffset; 
		i < mapData->_endOffset; 
		i += mapData->_blockSize, ++blocksFlushed) {
			mapData->_mapFile->_flushView(i, mapData->_blockSize);
			/*
			logger() << "Flushed {current: " <<i << ", startOffset: " << mapData->_startOffset
			<< ", blockSize: " << mapData->_blockSize << ", endOffset: " << mapData->_endOffset
			<< ", flushedBlocks: " << blocksFlushed
			<< "}" << endl;
			*/
	}

	logger() << "Ending Threaded Data Flusher {FlushedBlocks: " << blocksFlushed << "}" << endl;
	return 0;
}

void MMapFile::close() {
	if (_view) {
		UnmapViewOfFile(_view);
		_view = 0;
	}

	if (_mapHandle) {
		CloseHandle(_mapHandle);
	}
	_mapHandle = 0;

	if (_handle) {
		CloseHandle(_handle);
	}
	_handle = 0;
}

bool MMapFile::_flushView(unsigned long long offset, unsigned long long len) {
	void* startView = static_cast<char*>(_view) + offset;
	if (!FlushViewOfFile(startView, 0)) {
		logger() << "Failed flush view of file {file: " << _filename << ", startView: " << startView
			<< ", flushLen: " << len << ", code: " << GetLastError() << "}" << endl;
		return false;
	}
	return true;
}

bool MMapFile::flush(unsigned threads) {
	if (!_view || !_length) {
		return false;
	}

	if (threads <= 0) {
		//return _flushView(0, _length);
		return _flushView(0, 0);
	} else {
		DWORD* threadIds = new DWORD[threads];
		ThreadedFlushData* threadDataList = new ThreadedFlushData[threads];
		HANDLE* threadHandles = new HANDLE[threads];
	
		unsigned long long blockOffsets = (_length / threads) + _flushBlockSize - ((_length / threads) % _flushBlockSize);
		unsigned long long nextOffset = 0;
		for (unsigned i = 0; i < threads; ++i) {
			threadDataList[i]._mapFile = this;
			threadDataList[i]._blockSize = _flushBlockSize;
			threadDataList[i]._startOffset = nextOffset;
			threadDataList[i]._endOffset = (nextOffset + blockOffsets) > _length ? _length : (nextOffset + blockOffsets);
			nextOffset += blockOffsets;

			threadHandles[i] = CreateThread(NULL /* default security attributes */, 0 /* default stack size */,
				&MMapFile::_flushThreadedView, &threadDataList[i], 0 /* default flags */, &threadIds[i] /* threadId return */);
			if (threadHandles[i] == NULL) {
				logger() << "FATAL: Failed to create thread {i: " << i << "}" << endl;
			}
		}

		WaitForMultipleObjects(threads, threadHandles, TRUE, INFINITE);
		for (unsigned i = 0; i < threads; ++i) {
			CloseHandle(threadHandles[i]);
		}

		delete[] threadIds;
		delete[] threadDataList;
		delete[] threadHandles;
	}

	if (!FlushFileBuffers(_handle)) {
		logger() << "Failed flush file buffers {file: " << _filename 
			<< ", code: " << GetLastError() << "}" << endl;
		return false;
	}

	return true;
}
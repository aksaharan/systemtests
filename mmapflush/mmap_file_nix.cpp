#include <ctime>
#include <sstream>

#include "logger.h"
#include "mmap_file_nix.h"

/*
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
*/

namespace {
	int _flushThreadedViewNix(void* data) {
		MMapFile::_flushThreadedView(data);

	}
}


MMapFileNix::~MMapFileNix() {
	close();
}

DWORD MMapFile::_flushThreadedView(LPVOID data) {
	ThreadedFlushData* mapData = reinterpret_cast<ThreadedFlushData*>(data);
	logger() << "Called Threaded Data Flusher {mapFile: " << mapData->_mapFile->filename()
		<< ", length: " << mapData->_mapFile->length() << ", startOffset: " << mapData->_startOffset
		<< ", skipBlockSize: " << mapData->_skipBlockSize << ", flushBlockSize: " << mapData->_flushBlockSize
		<< ", endOffset: " << mapData->_endOffset << "}" << endl;

	unsigned long long blocksFlushed = 0;
	for (unsigned long long i = mapData->_startOffset; 
		i < mapData->_endOffset; 
		i += mapData->_skipBlockSize, ++blocksFlushed) {

			mapData->_mapFile->_flushView(i, mapData->_flushBlockSize);
			/*
			{
				logger() << "Flushed {current: " << i << ", startOffset: " << mapData->_startOffset
					<< ", blockSize: " << mapData->_blockSize << ", endOffset: " << mapData->_endOffset
					<< ", flushedBlocks: " << blocksFlushed
					<< "}" << endl;
			}
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
	if (!FlushViewOfFile(startView, len)) {
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

	ULONGLONG startTime = GetTickCount64();
	bool status = true;
	if (threads <= 0) {
		status = _flushView(0, 0);
	} else {
		DWORD* threadIds = new DWORD[threads];
		ThreadedFlushData* threadDataList = new ThreadedFlushData[threads];
		HANDLE* threadHandles = new HANDLE[threads];
	
		unsigned long long blockOffsets = (_length / threads) + _flushBlockSize - ((_length / threads) % _flushBlockSize);
		if (_adjBlockFlush) {
			// If adjacent block flush is enabled for threaded-flush, then make threads run in parallel
			// and flush the adjacent blocks of _flushBlockSize
			blockOffsets = _flushBlockSize * threads;
		}

		unsigned long long nextOffset = 0;
		for (unsigned i = 0; i < threads; ++i) {
			threadDataList[i]._mapFile = this;
			threadDataList[i]._flushBlockSize = _flushBlockSize;

			if (_adjBlockFlush) {
				// 'N" different adjacent blocks are being flushed in parallel by the threads till end
				threadDataList[i]._skipBlockSize = blockOffsets;
				threadDataList[i]._startOffset = _flushBlockSize * i;
				threadDataList[i]._endOffset = _length;
			} else {
				// 'N' Different block-sections of the file are being flushed in parallel
				threadDataList[i]._skipBlockSize = _flushBlockSize;
				threadDataList[i]._startOffset = nextOffset;
				threadDataList[i]._endOffset = (nextOffset + blockOffsets) > _length ? _length : (nextOffset + blockOffsets);
				nextOffset += blockOffsets;
			}

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

	logger() << "Completed flushing views of the file {file: " << _filename << ", length: " << _length << "}" << endl;

	ULONGLONG flushViewCompleteTime = GetTickCount64();
	if (!FlushFileBuffers(_handle)) {
		logger() << "Failed flush file buffers {file: " << _filename 
			<< ", code: " << GetLastError() << "}" << endl;
		return false;
	}

	ULONGLONG flushBufferCompleteTime = GetTickCount64();
	logger() << "Flushed data file {file: " << _filename << ", length: " << _length
		<< ", stats: {flushView: " << (flushViewCompleteTime - startTime) 
		<< ", flushBuffer: " << (flushBufferCompleteTime - flushViewCompleteTime)
		<< ", total: " << (flushBufferCompleteTime - startTime)
		<< "} }" << endl;

	return status;
}

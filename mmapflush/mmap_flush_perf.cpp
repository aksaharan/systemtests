#include <iostream>
#include <vector>
#include <ctime>

#include "mmap_file.h"
#include "mmap_file_mgr.h"

using namespace std;

void printHelp(char* progName) {
	cout << "Usage: " << progName << " <Type> <FileCreateOptions> <ZeroFill> <FlushThreadCount> <TouchOffsets> <TouchValue> <File1> [...<FileN>]" << endl
		<< "  FileCreateOptions    Integer value that can be passed for CreateFile" << endl
		<< endl;
}

int runMMapTest(int argc, char* argv[]) {
	logger() << "runMMapTest alled" << endl;
	if (argc <= 7) {
		printHelp(argv[0]);
		return 0;
	}

	logger() << "Parsing Options" << endl;
	long createOptions = atol(argv[2]);
	createOptions = createOptions ? createOptions : (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN);

	bool zeroFill = atol(argv[3]) ? true : false;
	long flushThreadCount = atol(argv[4]);
	flushThreadCount = flushThreadCount < 0 ? 0 : flushThreadCount;

	long touchOffset = atol(argv[5]);
	touchOffset += (touchOffset % sizeof(long)) ? (sizeof(long) - (touchOffset % sizeof(long))) : 0;
	touchOffset = (touchOffset <= 0 || touchOffset >= (20 * 1024 * 1024)) ? 1024 : touchOffset;

	long touchValue = atol(argv[6]);
	touchValue = touchValue ? touchValue : rand();

	vector<string> files;
	for (int i = 7; i < argc; ++i) {
		files.push_back(string(argv[i]));
	}

	logger() << "Running MMap file test for following configuration {" << endl
		<< "  createOptions: " << (void*)createOptions << endl
		<< "  zeroFill: " << zeroFill << endl
		<< "  flushThreadCount: " << flushThreadCount << endl
		<< "  touchOffset: " << touchOffset << endl
		<< "  touchValue: " << touchValue << endl
		<< "  files: " << files.size() << endl
		<< "}" << endl;

	MMapFileManager& mgr = MMapFileManager::getInstance();
	mgr.setFileCreateOptions(createOptions);
	mgr.setFlushThreads(flushThreadCount);
	mgr.setZeroFill(zeroFill);
	mgr.setParallelFlush(true);
	mgr.setUpdateOffset(touchOffset);
	mgr.setUpdateValue(touchValue);
	mgr.setFileSize(512LL * 1024 * 1024);

	mgr.runTests(files);
	return 0;
}

int main(int argc, char* argv[]) {
	srand(static_cast<unsigned int>(time(NULL)));
	if (argc <= 2) {
		printHelp(argv[0]);
		return 0;
	}

	if (!_stricmp(argv[1], "mmap")) {
		return runMMapTest(argc, argv);
	} else {
		logger() << "Unknown type info {" << argv[1] << "}" << endl;
		printHelp(argv[0]);
	}

	return 0;
}
#include <iostream>
#include <vector>
#include <ctime>

#include "program_options.h"
#include "logger.h"
#include "mmap_file.h"
#include "mmap_file_mgr.h"

using namespace std;

void printHelp(char* progName) {
	cout << "Usage: " << progName << " <Type> <FileCreateOptions> <ZeroFill> <FlushThreadCount> <TouchOffsets> <TouchValue> <ParallelFileFlush> <File1> [...<FileN>]" << endl
		<< "Type                  Types for test [mmap]" << endl
		<< "FileCreateOptions     Values passed for CreateFile - set 0 for default" << endl
		<< "ZeroFill              Fill new files wih zero - set 0 for default" << endl
		<< "FlushThreadCount      Number of threads to use flushing regions of mapped file" << endl
		<< "TouchOffsets          Periodic offsets that should be dirtied for the test" << endl
		<< "TouchValue            Value to set at dirty bytes - set 0 for one random value to be picked" << endl
		<< "ParallelFileFlush     Set 1 to enable flushing of multiple files in parallel else set - 0" << endl
		<< endl;
}

template<typename ValueType>
ValueType extractCmdLine(const string& cmdLineArg, const string& key, ValueType defaultValue) {
	return defaultValue;
}

int runMMapTest(int argc, char* argv[]) {
	logger() << "runMMapTest called" << endl;
	ProgramOptions programOptions(argc, (char**)argv);
	programOptions.append("type", false, "mmap")
		.append("foption", true, (long)(FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN))
		.append("zfill", true, true)
		.append("fthreads", true, (unsigned long)0)
		.append("pfflush", true, false)
		.append("uoffset", true, (unsigned long)(16 * 1024 - 1))
		.append("uvalue", true, (long)0)
		.append("fsize", true, (unsigned long long)(512LL * 1024 * 1024))
		;
	programOptions.parse(false);

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
	bool parallelFileFlush = atol(argv[7]) ? true : false;

	vector<string> files;
	for (int i = 8; i < argc; ++i) {
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
	mgr.setParallelFlush(parallelFileFlush);
	mgr.setUpdateOffset(touchOffset);
	mgr.setUpdateValue(touchValue);
	mgr.setFileSize(512LL * 1024 * 1024);

	if (!mgr.getNextMemoryMappedFileLocation(0) || !mgr.fetchMinOSPageSizeBytes()) {
		logger() << "Not supported platform/configuration, make sure you are running on 64-bit platform {mmap-offset: "
			<< mgr.getNextMemoryMappedFileLocation(0) << ", minOSPageSize: " << mgr.fetchMinOSPageSizeBytes()
			<< "}" << endl;
		return 1;
	}

	mgr.runTests(files);
	return 0;
}

int main(int argc, char* argv[]) {
	srand(static_cast<unsigned int>(time(NULL)));
	
	/*
	if (argc <= 2) {
		printHelp(argv[0]);
		return 0;
	}

	if (!_stricmp(argv[1], "mmap")) {
	*/
		return runMMapTest(argc, argv);
		/*
	} else {
		logger() << "Unknown type info {" << argv[1] << "}" << endl;
		printHelp(argv[0]);
	}
	*/

	return 0;
}
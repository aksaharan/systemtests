#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <string>

using namespace std;

int main(int argc, const char* argv[]) {
	if (argc <= 1) {
		cerr << "Usage: " << argv[0] << " <file>" << endl
			<< "   " << "Continuous read operations on a memory mapped file region" << endl
			;
		return 1;
	}

	string filename = argv[1];
	int fd = open(filename.c_str(), O_RDWR | O_EXLOCK);
	if (fd < 0) {
		cerr << " Failed to open the file [error: " << errno << "], will not continue further." << endl;
		return 2;
	}

	// 400 KB
	int mapSize = 1024;
	unsigned int sleepTime = 30;

	void* mappedAddress = mmap(NULL, mapSize, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 0);
	if (mappedAddress == MAP_FAILED) {
		cerr << "Failed to mmap the file [error: " << errno << "], will not continue further." << endl;
		goto close_file;
	}

	for (int i = 0; i < 1000; i++) {
		cout << "Iteration - " << i << " for reading the data from mmaped file...." << endl;
		for (int j = 0; j < mapSize; ++j) {
			char x = static_cast<char*>(mappedAddress)[j];
		}
		cout << "  Comeplete read iteration - sleeping now for " << sleepTime << " secs ......" << endl;
		sleep(sleepTime);
	}

unmap_file:
	if (munmap(mappedAddress, mapSize) < 0) {
		cerr << " Failed to unmap the file [error: " << errno << "], will continue with closing the file." << endl;
		return 2;
	}

close_file:
	close(fd);
	return 0;
}

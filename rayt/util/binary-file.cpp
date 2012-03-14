#include "binary-file.h"
using namespace std;
using namespace boost;

namespace rayt {
    
	BinaryFile::BinaryFile(const char *filename, bool read, bool write) {
		readable_ = read;
		writable_ = write;
		if (!read && !write) {
			file_ = NULL;
		} else {
			string mode;
			if (read && write)
				mode = "r+";
			else if (read)
				mode = 'r';
			else if (write)
				mode = 'w';
			mode += 'b';
			file_ = fopen(filename, mode.c_str());

			if (!file_)
				readable_ = writable_ = false;
		}
	}

	BinaryFile::~BinaryFile() {
		if (file_)
			fclose(file_);
	}

	bool BinaryFile::Readable() const {
		return readable_;
	}

	bool BinaryFile::Writable() const {
		return writable_;
	}
        
	bool BinaryFile::Read(long long start, int size, void *out) {
		if (!readable_)
			return false;

		if (_fseeki64(file_, start, SEEK_SET))
			return false;
		if (fread(out, 1, size, file_) != size)
			return false;
		return true;
	}

	bool BinaryFile::Write(long long start, int size, const void *in) {
		if (!writable_)
			return false;

		if (_fseeki64(file_, start, SEEK_SET))
			return false;
		if (fwrite(in, 1, size, file_) != size)
			return false;
		return true;
	}
    
}

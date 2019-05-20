#ifndef FILE__HXX
#define FILE__HXX

#include <cstdio>
#include <utility>
#include "stream.hxx"

struct file_t final : stream_t
{
private:
	FILE *file;

public:
	constexpr file_t() noexcept : file{nullptr} { }
	constexpr file_t(FILE *const _file) noexcept : file{_file} { }
	file_t(const char *const fileName, const char *mode) noexcept : file{fopen(fileName, mode)} { }
	file_t(file_t &&_file) : file_t{} { swap(_file); }
	~file_t() noexcept { if (file) fclose(file); }
	void operator =(file_t &&_file) noexcept { swap(_file); }
	operator FILE *() noexcept WARN_UNUSED { return file; }
	operator const FILE *() const noexcept WARN_UNUSED { return file; }
	bool operator ==(const FILE *const _file) const noexcept WARN_UNUSED { return file == _file; }
	bool valid() const noexcept WARN_UNUSED { return file; }
	bool atEOF() const noexcept final override WARN_UNUSED { return feof(file); }
	void swap(file_t &_file) noexcept { std::swap(file, _file.file); }

	bool read(void *const value, const size_t valueLen, size_t &resultLen) noexcept final override WARN_UNUSED
	{
		resultLen = fread(value, valueLen, 1, file);
		return resultLen == valueLen;
	}

	size_t read(void *const bufferPtr, const size_t len, std::nullptr_t) noexcept final override WARN_UNUSED
		{ return fread(bufferPtr, len, 1, file); }
	bool write(const void *const value, const size_t valueLen) noexcept final override WARN_UNUSED
		{ return fwrite(value, valueLen, 1, file) != valueLen; }
	size_t write(const void *const bufferPtr, const size_t len, std::nullptr_t) noexcept final override WARN_UNUSED
		{ return fwrite(bufferPtr, len, 1, file); }
	off_t seek(const off_t offset, const int32_t whence) const noexcept WARN_UNUSED
		{ return fseek(file, offset, whence); }
	off_t tell() const noexcept WARN_UNUSED { return seek(0, SEEK_CUR); }

#if 0
	stat_t stat() const noexcept WARN_UNUSED
	{
		stat_t fileStat{};
		if (!::fstat(fd, &fileStat))
			return fileStat;
		return {};
	}

	off_t length() const noexcept WARN_UNUSED
	{
		stat_t fileStat{};
		const int result = fstat(fd, &fileStat);
		return result ? -1 : fileStat.st_size;
	}
#endif

	bool seekRel(const off_t offset) const noexcept WARN_UNUSED
	{
		const off_t currentPos = tell();
		if (currentPos == -1 || currentPos + offset < 0)
			return false;
		return seek(offset, SEEK_CUR) == currentPos + offset;
	}

	file_t(const file_t &) = delete;
	file_t &operator =(const file_t &) = delete;
};

inline void swap(file_t &a, file_t &b) noexcept { a.swap(b); }

#endif /*FILE__HXX*/

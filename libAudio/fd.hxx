#ifndef FD__HXX
#define FD__HXX

#include <stdint.h>
#include <cstddef>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utility>
#include <memory>
#include "managedPtr.hxx"

#ifdef _MSVC
#define O_NOCTTY _O_BINARY
#endif

#ifdef __GNUC__
#define WARN_UNUSED		__attribute__((warn_unused_result))
#else
#define WARN_UNUSED
#endif

using stat_t = struct stat;

/*!
 * @internal
 * While this is supposed to be a very thin, RAII-only layer,
 * between a file descriptor and the code that uses it, due to the need to know
 * EOF outside of just read() calls, this also holds a flag for that express purpose.
 */
struct fd_t final
{
private:
	int32_t fd;
	mutable bool eof;

public:
	constexpr fd_t() noexcept : fd{-1}, eof{false} { }
	constexpr fd_t(const int32_t fd_) noexcept : fd{fd_}, eof{false} { }
	fd_t(const char *const file, const int flags, const mode_t mode = 0) noexcept :
		fd{::open(file, flags, mode)}, eof{false} { }
	fd_t(fd_t &&fd_) : fd_t{} { swap(fd_); }
	~fd_t() noexcept { if (fd != -1) close(fd); }
	void operator =(fd_t &&fd_) noexcept { swap(fd_); }
	operator int32_t() const noexcept WARN_UNUSED { return fd; }
	bool operator ==(const int32_t desc) const noexcept WARN_UNUSED { return fd == desc; }
	bool valid() const noexcept WARN_UNUSED { return fd != -1; }
	bool isEOF() const noexcept WARN_UNUSED { return eof; }
	void swap(fd_t &desc) noexcept
	{
		std::swap(fd, desc.fd);
		std::swap(eof, desc.eof);
	}

	bool read(void *const value, const size_t valueLen) const noexcept WARN_UNUSED
	{
		size_t actualLen;
		return read(value, valueLen, actualLen);
	}

	bool read(void *const value, const size_t valueLen, size_t &actualLen) const noexcept WARN_UNUSED
	{
		actualLen = 0;
		if (eof)
			return false;
		const ssize_t result = ::read(fd, value, valueLen);
		if (result == 0 && valueLen != 0)
			eof = true;
		else if (result > 0)
			actualLen = size_t(result);
		return actualLen == valueLen;
	}

	ssize_t read(void *const bufferPtr, const size_t len, std::nullptr_t) const noexcept WARN_UNUSED
		{ return ::read(fd, bufferPtr, len); }
	ssize_t write(const void *const bufferPtr, const size_t len) const noexcept WARN_UNUSED
		{ return ::write(fd, bufferPtr, len); }
	off_t seek(off_t offset, int32_t whence) const noexcept WARN_UNUSED { return ::lseek(fd, offset, whence); }
	off_t tell() const noexcept WARN_UNUSED { return seek(0, SEEK_CUR); }
	fd_t dup() const noexcept WARN_UNUSED { return ::dup(fd); }

	off_t length() const noexcept WARN_UNUSED
	{
		struct stat fileStat;
		const int result = fstat(fd, &fileStat);
		return result ? -1 : fileStat.st_size;
	}

	template<typename T> bool read(T &value) const noexcept
		{ return read(&value, sizeof(T)); }
	template<typename T, size_t N> bool read(std::array<T, N> &value) const noexcept
		{ return read(value.data(), N * sizeof(T)); }
	template<typename T> bool read(const std::unique_ptr<T> &value) const noexcept
		{ return read(value.get(), sizeof(T)); }
	template<typename T> bool read(std::unique_ptr<T []> &value, const size_t valueCount) const noexcept
		{ return read(value.get(), sizeof(T) * valueCount); }
	template<typename T> bool read(const managedPtr_t<T> &value, const size_t valueLen) const noexcept
		{ return read(value.get(), valueLen); }
	bool read(const managedPtr_t<void> &value, const size_t valueLen) const noexcept WARN_UNUSED
		{ return read(value.get(), valueLen); }

	template<size_t length, typename T, size_t N> bool read(std::array<T, N> &value) const noexcept
	{
		static_assert(length <= N, "Can't request to read more than the std::array<> length");
		return read(value.data(), sizeof(T) * length);
	}

	fd_t(const fd_t &) = delete;
	fd_t &operator =(const fd_t &) = delete;
};

inline void swap(fd_t &a, fd_t &b) noexcept { a.swap(b); }
constexpr mode_t normalMode = S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH;

#endif /*FD__HXX*/

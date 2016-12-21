#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <utility>

#ifdef _MSVC
#define O_NOCTTY _O_BINARY
#endif

#ifdef __GNUC__
#define WARN_UNUSED		__attribute__((warn_unused_result))
#else
#define WARN_UNUSED
#endif

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

	template<typename T, typename U = T> inline T exchange(T &obj, U &&newVal)
    {
		T oldVal = std::move(obj);
		obj = std::forward<U>(newVal);
		return oldVal;
	}

	int32_t release() noexcept
	{
		return exchange(fd, -1);
		/*int32_t desc = fd;
		fd = -1;
		return desc;*/
	}

	void reset(const int32_t desc = -1) noexcept
	{
		if (fd != -1)
			close(fd);
		fd = desc;
	}

public:
	constexpr fd_t() noexcept : fd(-1), eof(false) { }
	constexpr fd_t(const int32_t s) noexcept : fd(s), eof(false) { }
	fd_t(const char *const file, int flags, mode_t mode = 0) noexcept : fd(::open(file, flags, mode)), eof(false) { }
	fd_t(fd_t &&s) noexcept : fd(s.release()), eof(s.eof) { }
	~fd_t() noexcept { reset(); }

	fd_t &operator =(fd_t &&s) noexcept
	{
		swap(s);
		return *this;
	}

	operator int32_t() const noexcept WARN_UNUSED { return fd; }
	bool operator ==(const int32_t desc) const noexcept WARN_UNUSED { return fd == desc; }
	bool valid() const noexcept WARN_UNUSED { return fd != -1; }
	bool isEOF() const noexcept WARN_UNUSED { return eof; }
	void swap(fd_t &desc) noexcept
	{
		std::swap(fd, desc.fd);
		std::swap(eof, desc.eof);
	}

	ssize_t read(void *const bufferPtr, const size_t len) const noexcept WARN_UNUSED
	{
		if (eof)
			return 0;
		const ssize_t result = ::read(fd, bufferPtr, len);
		if (result == 0)
			eof = true;
		return result;
	}
	ssize_t write(const void *const bufferPtr, const size_t len) const noexcept WARN_UNUSED { return ::write(fd, bufferPtr, len); }
	off_t seek(off_t offset, int32_t whence) const noexcept WARN_UNUSED { return ::lseek(fd, offset, whence); }
	off_t tell() const noexcept WARN_UNUSED { return seek(0, SEEK_CUR); }

	fd_t(const fd_t &) = delete;
	fd_t &operator =(const fd_t &) = delete;
};

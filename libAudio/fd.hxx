#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <utility>

#ifdef _MSVC
#define O_NOCTTY _O_BINARY
#endif

struct fd_t final
{
private:
	int32_t fd;

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
	constexpr fd_t() noexcept : fd(-1) { }
	constexpr fd_t(const int32_t s) noexcept : fd(s) { }
	fd_t(const char *const file, int flags, mode_t mode = 0) noexcept : fd(::open(file, flags, mode)) { }
	fd_t(fd_t &&s) noexcept : fd(s.release()) { }
	~fd_t() noexcept { reset(); }

	fd_t &operator =(fd_t &&s) noexcept
	{
		swap(s);
		return *this;
	}

	operator int32_t() const noexcept { return fd; }
	bool operator ==(const int32_t desc) const noexcept { return fd == desc; }
	void swap(fd_t &desc) noexcept { std::swap(fd, desc.fd); }
	bool valid() const noexcept { return fd != -1; }

	ssize_t read(void *const bufferPtr, const size_t len) const noexcept { return ::read(fd, bufferPtr, len); }
	ssize_t write(const void *const bufferPtr, const size_t len) const noexcept { return ::write(fd, bufferPtr, len); }

	fd_t(const fd_t &) = delete;
	fd_t &operator =(const fd_t &) = delete;
};

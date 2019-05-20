#ifndef STREAM__HXX
#define STREAM__HXX

#include <array>
#include <memory>
#include <string>

#ifndef WARN_UNUSED
#ifdef __GNUC__
#define WARN_UNUSED		__attribute__((warn_unused_result))
#else
#define WARN_UNUSED
#endif
#endif

struct stream_t
{
public:
	stream_t() = default;
	stream_t(stream_t &&) = default;
	virtual ~stream_t() = default;
	stream_t &operator =(stream_t &&) = default;

	template<typename T> bool read(T &value) noexcept
		{ return read(&value, sizeof(T)); }
	template<typename T, size_t N> bool read(std::array<T, N> &value) noexcept
		{ return read(value.data(), N * sizeof(T)); }
	template<typename T> bool read(const std::unique_ptr<T> &value) noexcept
		{ return read(value.get(), sizeof(T)); }
	template<typename T> bool read(std::unique_ptr<T []> &value, const size_t valueCount) noexcept
		{ return read(value.get(), sizeof(T) * valueCount); }

	template<size_t length, typename T, size_t N> bool read(std::array<T, N> &value) noexcept
	{
		static_assert(length <= N, "Can't request to read more than the std::array<> length");
		return read(value.data(), sizeof(T) * length);
	}

	template<typename T> bool write(const T &value) noexcept
		{ return write(&value, sizeof(T)); }
	template<typename T, size_t N> bool write(const std::array<T, N> &value) noexcept
		{ return write(value.data(), N * sizeof(T)); }
	template<typename T> bool write(const std::unique_ptr<T> &value) noexcept
		{ return write(value.get(), sizeof(T)); }
	template<typename T> bool write(const std::unique_ptr<T []> &value, const size_t valueCount) noexcept
		{ return write(value.get(), sizeof(T) * valueCount); }
	bool write(const std::string &value) noexcept WARN_UNUSED
		{ return write(value.data(), value.length()); }

	template<size_t length, typename T, size_t N> bool write(const std::array<T, N> &value) noexcept
	{
		static_assert(length <= N, "Can't request to write more than the std::array<> length");
		return write(value.data(), sizeof(T) * length);
	}

	bool read(void *const value, const size_t valueLen) noexcept WARN_UNUSED
	{
		size_t resultLen = 0;
		return read(value, valueLen, resultLen);
	}

	virtual bool read(void *const, const size_t, size_t &) noexcept WARN_UNUSED = 0;
	virtual size_t read(void *const, const size_t, std::nullptr_t) noexcept WARN_UNUSED = 0;
	virtual bool write(const void *const, const size_t) noexcept WARN_UNUSED = 0;
	virtual size_t write(const void *const, const size_t, std::nullptr_t) noexcept WARN_UNUSED = 0;
	virtual bool atEOF() const noexcept WARN_UNUSED = 0;

	stream_t(const stream_t &) = delete;
	stream_t &operator =(const stream_t &) = delete;
};

#endif /*STREAM__HXX*/
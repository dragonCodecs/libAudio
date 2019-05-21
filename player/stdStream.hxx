#ifndef STD_STREAM__HXX
#define STD_STREAM__HXX

#ifdef _WINDOWS
#include <stringapiset.h>
#include <uniquePtr.hxx>
#endif
#include "file.hxx"

// WARNING: This assumes you're giving it a TEXT stream so no non-printable stuff you want to preserve.
// It will (if necessary) automatically UTF-8 => 16 convert whatever passes through for the sake of windows
struct stdStream_t final : stream_t
{
private:
	file_t stream;

public:
	constexpr stdStream_t() noexcept : stream{} { }
	constexpr stdStream_t(FILE *const file) noexcept : stream{file} { }
	stdStream_t(const char *const fileName, const char *mode) noexcept : stream{fileName, mode} { }
	stdStream_t(stdStream_t &&_file) : stdStream_t{} { swap(_file); }
	~stdStream_t() noexcept final override = default;
	void operator =(stdStream_t &&stream) noexcept { swap(stream); }
	operator FILE *() noexcept WARN_UNUSED { return stream; }
	operator const FILE *() const noexcept WARN_UNUSED { return stream; }
	bool operator ==(const FILE *const file) const noexcept WARN_UNUSED { return stream == file; }
	bool valid() const noexcept WARN_UNUSED { return stream.valid(); }
	bool atEOF() const noexcept final override WARN_UNUSED { return stream.atEOF(); }
	void swap(stdStream_t &_stream) noexcept { stream.swap(_stream.stream); }

	bool read(void *const, const size_t, size_t &) noexcept final override WARN_UNUSED { return false; }
	size_t read(void *const, const size_t, std::nullptr_t) noexcept final override WARN_UNUSED { return 0; }

	bool write(const void *const value, const size_t valueLen) noexcept final override WARN_UNUSED
	{
#ifdef _WINDOWS
		const size_t stringLen = MultiByteToWideChar(CP_UTF8, 0, value, valueLen, nullptr, 0);
		auto string = makeUnique<wchar_t>(stringLen);
		if (!string)
			return false;
		MultiByteToWideChar(CP_UTF8, 0, value, valueLen, string.get(), stringLen);
		return stream.write(string.get(), sizeof(wchar_t) * stringLen);
#else
		return stream.write(value, valueLen);
#endif
	}

	size_t write(const void *const value, const size_t valueLen, std::nullptr_t) noexcept final override WARN_UNUSED
	{
#ifdef _WINDOWS
		const size_t stringLen = MultiByteToWideChar(CP_UTF8, 0, value, valueLen, nullptr, 0);
		auto string = makeUnique<wchar_t>(stringLen);
		if (!string)
			return 0;
		MultiByteToWideChar(CP_UTF8, 0, value, valueLen, string.get(), stringLen);
		return stream.write(string.get(), sizeof(wchar_t) * stringLen, nullptr);
#else
		return stream.write(value, valueLen, nullptr);
#endif
	}

	stdStream_t(const stdStream_t &) = delete;
	stdStream_t &operator =(const stdStream_t &) = delete;
};

#endif /*STD_STREAM__HXX*/
#include <cerrno>
#ifndef _WINDOWS
#include <unistd.h>
#else
#include <stringapiset.h>
#include <io.h>
#include "uniquePtr.hxx"
#endif
#include "console.hxx"

using namespace libAudio::console;
using charTraits = std::char_traits<char>;

console_t console;

void consoleStream_t::checkTTY() noexcept { _tty = isatty(fd); }

void consoleStream_t::write(const void *const buffer, const size_t bufferLen) const noexcept
{
	// We don't actually care if this succeeds. We just try if at all possible.
	(void)::write(fd, buffer, bufferLen);
	errno = 0; // extra insurance.
}

// WARNING: This assumes you're giving it a TEXT stream so no non-printable stuff you want to preserve.
// It will (if necessary) automatically UTF-8 => 16 convert whatever passes through for the sake of windows
void consoleStream_t::write(const char *const value) const noexcept
{
	if (value)
	{
#ifdef _WINDOWS
		const size_t valueLen = charTraits::length(value);
		const size_t stringLen = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED | MB_USEGLYPHCHARS,
			value, valueLen, nullptr, 0);
		auto string = makeUnique<wchar_t>(stringLen);
		if (!string)
			return;
		MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED | MB_USEGLYPHCHARS, value, valueLen,
			string.get(), stringLen);
		write(string.get(), sizeof(wchar_t) * stringLen);
#else
		write(value, charTraits::length(value));
#endif
	}
	else
		write("(null)"_s);
}

void consoleStream_t::write(const bool value) const noexcept
{
	if (value)
		write("true"_s);
	else
		write("false"_s);
}

console_t::console_t(FILE *const outStream, FILE *const errStream) noexcept :
	outputStream{fileno(outStream)}, errorStream{fileno(errStream)},
	valid{outputStream.valid() && errorStream.valid()} { }

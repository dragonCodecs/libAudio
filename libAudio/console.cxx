#include <cerrno>
#ifndef _WINDOWS
#include <unistd.h>
#else
#include <io.h>
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

void consoleStream_t::write(const char *const value) const noexcept
{
	if (value)
		write(value, charTraits::length(value));
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

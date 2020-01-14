#include <cerrno>
#ifndef _WINDOWS
#include <unistd.h>
#else
#include <io.h>
#endif
#include "console.hxx"

using namespace libAudio::console;

console_t console;

void consoleStream_t::checkTTY() noexcept { _tty = isatty(fd); }

void consoleStream_t::write(const void *const buffer, const size_t bufferLen) const noexcept
{
	// We don't actually care if this succeeds. We just try if at all possible.
	(void)::write(fd, buffer, bufferLen);
	errno = 0; // extra insurance.
}

console_t::console_t(FILE *const outStream, FILE *const errStream) noexcept :
	outputStream{fileno(outStream)}, errorStream{fileno(errStream)},
	valid{outputStream.valid() && errorStream.valid()} { }

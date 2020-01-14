#ifndef CONSOLE__HXX
#define CONSOLE__HXX

#include <stdio.h>
#include <array>
#include "libAudio.h"

namespace libAudio
{
	namespace console
	{
		struct consoleStream_t final
		{
		private:
			int32_t fd;
			bool _tty;

			void checkTTY() noexcept;

		public:
			constexpr consoleStream_t() noexcept : fd{-1}, _tty{false} { }
			consoleStream_t(const int32_t desc) noexcept : fd{desc} { checkTTY(); }
			bool valid() const noexcept { return fd != -1; }
			bool isTTY() const noexcept { return _tty; }
			void write(const void *const buffer, const size_t bufferLen) const noexcept;

			template<typename T> void write(const T &value) const noexcept
				{ write(&value, sizeof(T)); }
			template<typename T, size_t N> void write(const std::array<T, N> &value) const noexcept
				{ write(value.data(), sizeof(T) * N); }
		};

		struct libAUDIO_CLS_API console_t final
		{
		private:
			consoleStream_t outputStream;
			consoleStream_t errorStream;
			bool valid;

		public:
			constexpr console_t() noexcept : outputStream{}, errorStream{}, valid{false} { }
			console_t(FILE *const outStream, FILE *const errStream) noexcept;
		};
	}
}

libAUDIO_CXX_API libAudio::console::console_t console;

#endif /*CONSOLE__HXX*/

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

		public:
			constexpr consoleStream_t() noexcept : fd{-1} { }
			consoleStream_t(const int32_t desc) noexcept : fd{desc} { }
			bool valid() const noexcept { return fd != -1; }
			void write(void *const buffer, const size_t bufferLen) const noexcept;

			template<typename T> bool write(const T &value)
				{ return write(&value, sizeof(T)); }
			template<typename T, size_t N> bool write(const std::array<T, N> &value)
				{ return write(value.data(), sizeof(T) * N); }
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

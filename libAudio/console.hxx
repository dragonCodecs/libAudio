#ifndef CONSOLE__HXX
#define CONSOLE__HXX

#include <cstdio>
#include <array>
#include <type_traits>
#include <cstring>
#include "libAudio.h"

namespace libAudio
{
	namespace console
	{
		template<bool B, typename T = void> using enableIf = typename std::enable_if<B, T>::type;
		template<typename T> using isIntegral = std::is_integral<T>;
		template<typename base_t, typename derived_t> using isBaseOf = std::is_base_of<base_t, derived_t>;

		template<typename> struct isChar : std::false_type { };
		template<> struct isChar<char> : std::true_type { };

		template<typename> struct __isBoolean : public std::false_type { };
		template<> struct __isBoolean<bool> : public std::true_type { };
		template<typename T> struct isBoolean : public std::integral_constant<bool,
			__isBoolean<typename std::remove_cv<T>::type>::value> { };

		template<typename T> struct isScalar : public std::integral_constant<bool,
			isIntegral<T>::value && !isBoolean<T>::value && !isChar<T>::value> { };

		struct consoleStream_t;

		struct printable_t
		{
			virtual void operator()(const consoleStream_t &) const noexcept;
			virtual ~printable_t() noexcept = default;
		};

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

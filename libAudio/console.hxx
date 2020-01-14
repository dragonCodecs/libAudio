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

		template<typename> struct asInt_t;

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
			void write(const char *const value) const noexcept { write(value, value ? strlen(value) : 0); }

			template<typename T> enableIf<isBaseOf<printable_t, T>::value> write(T &&printable) const noexcept
				{ printable(*this); }
			template<typename T> enableIf<isScalar<T>::value> write(const T value) const noexcept
				{ write(asInt_t<T>{value}); }
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

			template<typename T, typename... U> void write(const consoleStream_t &stream,
				T &&value, U &&...values) const noexcept
			{
				stream.write(value);
				write(stream, std::forward<U>(values)...);
			}

		public:
			constexpr console_t() noexcept : outputStream{}, errorStream{}, valid{false} { }
			console_t(FILE *const outStream, FILE *const errStream) noexcept;

			template<typename... T> void error(T &&...values) const noexcept
				{ write(errorStream, std::forward<T>(values)...); }

			template<typename... T> void info(T &&...values) const noexcept
				{ write(outputStream, std::forward<T>(values)...); }

			template<typename... T> void debug(T &&...values) const noexcept
				{ write(outputStream, std::forward<T>(values)...); }

			void dumpBuffer();
		};

		template<typename int_t> struct asInt_t : public printable_t
		{
		private:
			using uint_t = typename std::make_unsigned<int_t>::type;
			int_t _value;

			[[gnu::noinline]] uint_t format(const consoleStream_t &stream, const uint_t number) const noexcept
			{
				if (number < 10)
					stream.write(char(number + '0'));
				else
				{
					const char value = number - format(stream, number / 10) * 10 + '0';
					stream.write(value);
				}
				return number;
			}

			template<typename T> enableIf<std::is_same<T, int_t>::value &&
				isIntegral<T>::value && !isBoolean<T>::value && std::is_unsigned<T>::value>
				printTo(const consoleStream_t &stream) const noexcept { format(stream, _value); }

			template<typename T> [[gnu::noinline]] enableIf<std::is_same<T, int_t>::value &&
				isIntegral<T>::value && !isBoolean<T>::value && std::is_signed<T>::value>
				printTo(const consoleStream_t &stream) const noexcept
			{
				if (_value < 0)
				{
					stream.write('-');
					format(stream, ~uint_t(_value) - 1);
				}
				else
					format(stream, uint_t(_value));
			}

		public:
			constexpr asInt_t(const int_t value) noexcept : _value{value} { }
			void operator()(const consoleStream_t &stream) const noexcept final
				{ printTo<int_t>(stream); }
		};
	}

	using console::printable_t;
}

libAUDIO_CXX_API libAudio::console::console_t console;

#endif /*CONSOLE__HXX*/

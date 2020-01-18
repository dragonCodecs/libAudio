#ifndef CONSOLE__HXX
#define CONSOLE__HXX

#include <cstdio>
#include <cstddef>
#include <array>
#include <type_traits>
#include <string>
#include <memory>
#include <cstring>
#include "libAudio.h"

namespace libAudio
{
	namespace console
	{
		inline std::string operator ""_s(const char *str, const size_t len) noexcept { return {str, len}; }

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

		struct libAUDIO_CLS_API printable_t
		{
			virtual void operator()(const consoleStream_t &) const noexcept = 0;
			virtual ~printable_t() noexcept = default;
		};

		struct libAUDIO_CLS_API consoleStream_t final
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
			void write(const char *const value) const noexcept;
			void write(const char value) const noexcept { write(&value, 1); }
			template<typename T> void write(const std::unique_ptr<T> &value) const noexcept
				{ value ? write(*value) : write("(null)"_s); }
			template<typename T> void write(const std::unique_ptr<T []> &value) const noexcept { write(value.get()); }
			void write(const std::string &value) const noexcept
				{ write(value.data(), value.length()); }
			template<typename T, typename = enableIf<isBaseOf<printable_t, T>::value>>
				void write(T &&printable) const noexcept { printable(*this); }
			template<typename T, typename = enableIf<isScalar<T>::value>>
				void write(const T value) const noexcept;
			template<typename T> void write(const T *const ptr) const noexcept;
			void write(const bool value) const noexcept;
			template<size_t N> void write(const std::array<char, N> &value) const noexcept;
			template<typename T, size_t N> void write(const std::array<T, N> &value) const noexcept;
		};

		struct libAUDIO_CLS_API console_t final
		{
		private:
			consoleStream_t outputStream;
			consoleStream_t errorStream;
			bool valid;

			void write(const consoleStream_t &stream) const noexcept { stream.write('\n'); }
			void write(const consoleStream_t &, std::nullptr_t) const noexcept { }
			template<typename T, typename... U> void write(const consoleStream_t &stream,
				T &&value, U &&...values) const noexcept
			{
				stream.write(std::forward<T>(value));
				write(stream, std::forward<U>(values)...);
			}

			void _error() const noexcept;
			void _info() const noexcept;
			void _debug() const noexcept;

		public:
			constexpr console_t() noexcept : outputStream{}, errorStream{}, valid{false} { }
			libAUDIO_CLS_API console_t(FILE *const outStream, FILE *const errStream) noexcept;

			template<typename... T> libAUDIO_CLS_API void error(T &&...values) const noexcept
				{ _error(); write(errorStream, std::forward<T>(values)...); }

			template<typename... T> libAUDIO_CLS_API void info(T &&...values) const noexcept
				{ _info(); write(outputStream, std::forward<T>(values)...); }

			template<typename... T> libAUDIO_CLS_API void debug(T &&...values) const noexcept
				{ _debug(); write(outputStream, std::forward<T>(values)...); }

			void dumpBuffer();
		};

		template<typename int_t> libAUDIO_CLS_API struct asInt_t final : public printable_t
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

		template<uint8_t padding = 0, char paddingChar = ' '>
			libAUDIO_CLS_API struct asHex_t final : public printable_t
		{
		private:
			uint8_t maxDigits;
			uint8_t msbShift;
			uintmax_t _value;

		public:
			template<typename T, typename = enableIf<std::is_unsigned<T>::value>>
				constexpr asHex_t(const T value) noexcept : maxDigits{sizeof(T) * 2},
				msbShift(4 * (maxDigits - 1)), _value(value) { }

			[[gnu::noinline]]
			void operator ()(const consoleStream_t &stream) const noexcept final
			{
				uintmax_t value{_value};
				// If we've been asked to pad by more than the maximum possible length of the number
				if (maxDigits < padding)
				{
					// Put out the excess padding early to keep the logic simple.
					for (uint8_t i{0}; i < padding - maxDigits; ++i)
						stream.write(paddingChar);
				}

				uint8_t digits{maxDigits};
				// For up to the maximum number of digits, pad as needed
				for (; digits > 1; --digits)
				{
					const uint8_t nibble = uint8_t((value >> msbShift) & 0x0FU);
					if (nibble == 0)
						value <<= 4;
					if (digits > padding && nibble == 0)
						continue;
					else if (digits <= padding && nibble == 0)
						stream.write(paddingChar);
					else
						break;
					// if 0 and padding == 0, we don't output anything here.
				}

				for (; digits > 0; --digits)
				{
					const uint8_t nibble = uint8_t((value >> msbShift) & 0x0FU);
					const char digit = nibble + '0';
					if (digit > '9')
						stream.write(char(digit + 7));
					else
						stream.write(digit);
					value <<= 4;
				}
			}
		};

		template<typename T, typename> void consoleStream_t::write(const T value) const noexcept
			{ write(asInt_t<T>{value}); }

		template<typename T> void consoleStream_t::write(const T *const ptr) const noexcept
		{
			write("0x"_s);
			write(asHex_t<8, '0'>{
				reinterpret_cast<uintptr_t>(ptr) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) lgtm[cpp/reinterpret-cast]
			});
		}

		template<size_t N> void consoleStream_t::write(const std::array<char, N> &value) const noexcept
		{
			for (const auto &elem : value)
				write(elem);
		}

		template<typename T, size_t N> void consoleStream_t::write(const std::array<T, N> &value) const noexcept
		{
			for (const auto &elem : value)
				write(asHex_t<sizeof(T) * 2, '0'>{elem});
		}

		struct libAUDIO_CLS_API asTime_t final : public printable_t
		{
		private:
			uint64_t value;

		public:
			constexpr asTime_t(const uint64_t _value) noexcept : value{_value} { }

			void operator ()(const consoleStream_t &stream) const noexcept final
			{
				asInt_t<uint64_t>{value / 60}(stream);
				stream.write("m "_s);
				asInt_t<uint64_t>{value % 60}(stream);
				stream.write("s"_s);
			}
		};
	}

	using console::printable_t;
}

libAUDIO_CXX_API libAudio::console::console_t console;

#endif /*CONSOLE__HXX*/

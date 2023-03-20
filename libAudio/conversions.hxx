// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2023 Rachel Mant <git@dragonmux.network>
namespace libAudio
{
	namespace conversions
	{
		inline namespace impl
		{
			template<typename A> using isSigned = std::is_signed<A>;
			template<typename A> using makeUnsigned = typename std::make_unsigned<A>::type;
		}

		constexpr bool isNumber(const char x) noexcept { return x >= '0' && x <= '9'; }
		constexpr bool isHex(const char x) noexcept { return isNumber(x) || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F'); }
		constexpr bool isOct(const char x) noexcept { return x >= '0' && x <= '7'; }

		template<typename int_t> struct toInt_t
		{
		private:
			using uint_t = makeUnsigned<int_t>;
			const char *const _value;
			const size_t _length;
			constexpr static bool _isSigned = isSigned<int_t>::value;

			template<bool isFunc(const char)> bool checkValue() const noexcept
			{
				for (size_t i = 0; i < _length; ++i)
				{
					if (!isFunc(_value[i]))
						return false;
				}
				return true;
			}

		public:
			toInt_t(const char *const value) noexcept : _value(value), _length(std::char_traits<char>::length(value)) { }
			constexpr toInt_t(const char *const value, const size_t subLength) noexcept : _value(value), _length(subLength) { }
			size_t length() const noexcept { return _length; }

			bool isInt() const noexcept
			{
				if (_isSigned && _value[0] == '-' && length() == 1)
					return false;
				for (size_t i = 0; i < _length; ++i)
				{
					if (_isSigned && i == 0 && _value[i] == '-')
						continue;
					else if (!isNumber(_value[i]))
						return false;
				}
				return true;
			}

			bool isHex() const noexcept { return checkValue<isHex>(); }
			bool isOct() const noexcept { return checkValue<isOct>(); }
			int_t fromInt() const noexcept { return *this; }

#ifdef _MSC_VER
// The `-int_t(value)` line in the next chunk of code
// generates this warning for unsigned types, even though
// the line is unreachable dead-code in this situation
#pragma warning(disable:4146)
#endif
			operator int_t() const noexcept
			{
				uint_t value(0);
				for (size_t i(0); i < _length; ++i)
				{
					if (_isSigned && i == 0 && _value[i] == '-')
						continue;
					value *= 10;
					value += _value[i] - '0';
				}
				if (_isSigned && _value[0] == '-')
					return -int_t(value);
				return int_t(value);
			}
#ifdef _MSC_VER
// Put the warning back how we found it..
#pragma warning(default:4146)
#endif

			int_t fromHex() const noexcept
			{
				int_t value(0);
				for (size_t i(0); i < _length; ++i)
				{
					uint8_t hex(_value[i]);
					if (!isHex(hex))
						return {};
					else if (hex >= 'a' && hex <= 'f')
						hex -= 0x20;
					value <<= 4U;
					hex -= 0x30;
					if (hex > 9)
						hex -= 0x07;
					value += hex;
				}
				return value;
			}

			int_t fromOct() const noexcept
			{
				int_t value(0);
				for (size_t i(0); i < _length; ++i)
				{
					if (!isOct(_value[i]))
						return {};
					value <<= 3U;
					value += _value[i] - 0x30;
				}
				return value;
			}
		};
	}
}

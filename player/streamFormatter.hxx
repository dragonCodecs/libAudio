#ifndef STREAM_FORMATTER__HXX
#define STREAM_FORMATTER__HXX

#include <cstdint>
#include <type_traits>
#include <utility>
#include "stream.hxx"

std::string operator ""_s(const char *const value, const size_t length) noexcept { return {value, length}; }
struct printable_t { };

template<typename> struct isBoolean_ : public std::false_type { };
template<> struct isBoolean_<bool> : public std::true_type { };
template<typename T> struct isBoolean : public std::integral_constant<bool,
	isBoolean_<typename std::remove_cv<T>::type>::value> { };

template<typename> struct isChar : std::false_type { };
template<> struct isChar<char> : std::true_type { };
template<typename T> struct isScalar : public std::integral_constant<bool,
	std::is_integral<T>::value && !isBoolean<T>::value && !isChar<T>::value> { };

template<uint8_t pad = 0, uint8_t padChar = ' '> struct asHex_t final : public printable_t
{
private:
	static constexpr uint8_t padding = pad;
	const uint8_t maxDigits;
	const uint8_t msbShift;
	const uint32_t number;

public:
	template<typename T> constexpr asHex_t(const T value) noexcept : maxDigits(sizeof(T) * 2),
		msbShift(4 * (maxDigits - 1)), number(value) { }

	[[gnu::noinline]]
	void operator ()(stream_t &stream) const noexcept
	{
		uint8_t i;
		uint32_t value(number);
		// If we've been asked to pad by more than the maximum possible length of the number
		if (maxDigits < pad)
		{
			// Put out the excess padding early to keep the logic simple.
			for (i = 0; i < (pad - maxDigits); ++i)
				stream.write(padChar);
		}

		// For up to the maximum number of digits, pad as needed
		for (i = maxDigits; i > 1; i--)
		{
			const uint8_t nibble = uint8_t((value >> msbShift) & 0x0F);
			if (nibble == 0)
				value <<= 4;
			if (i > pad && nibble == 0)
				continue;
			else if (i <= pad && nibble == 0)
				stream.write(padChar);
			else
				break;
			// If 0 and pad == 0, we don't output anything here.
		}

		for (; i > 0; --i)
		{
			const uint8_t nibble = uint8_t((value >> msbShift) & 0x0F);
			const uint8_t ch = nibble + '0';
			if (ch > '9')
				stream.write(ch + 7);
			else
				stream.write(ch);
			value <<= 4;
		}
	}
};

template<typename N> struct asInt_t final : public printable_t
{
private:
	typedef typename std::make_unsigned<N>::type uint_t;
	const N number;

	[[gnu::noinline]] uint_t print(const uint_t number, stream_t &stream) const noexcept
	{
		if (number < 10)
			stream.write(number + '0');
		else
		{
			const uint_t num = number - (print(number / 10, stream) * 10);
			stream.write(num + '0');
		}
		return number;
	}

	template<typename T> typename std::enable_if<std::is_same<T, N>::value && std::is_integral<T>::value &&
		!isBoolean<T>::value && std::is_unsigned<T>::value>::type format(stream_t &stream) const noexcept
		{ print(number, stream); }

	template<typename T> [[gnu::noinline]] typename std::enable_if<std::is_same<T, N>::value &&
		std::is_integral<T>::value && !isBoolean<T>::value && std::is_signed<T>::value>::type
		format(stream_t &stream) const noexcept
	{
		if (number < 0)
		{
			stream.write('-');
			print((typename std::make_unsigned<N>::type)-number, stream);
		}
		else
			print((typename std::make_unsigned<N>::type)number, stream);
	}

public:
	constexpr asInt_t(const N value) noexcept : number(value) { }
	void operator ()(stream_t &stream) const noexcept { format<N>(stream); }
};

struct asTime_t final : public printable_t
{
private:
	const uint64_t value;

public:
	constexpr asTime_t(const uint64_t _value) noexcept : value{_value} { }

	void operator ()(stream_t &stream) const noexcept
	{
		asInt_t<uint64_t>{value / 60}(stream);
		stream.write("m "_s);
		asInt_t<uint64_t>{value % 60}(stream);
		stream.write("s"_s);
	}
};

struct streamFormatter_t final
{
private:
	stream_t &stream;
	using charTraits = std::char_traits<char>;

	void print(const char *value) noexcept
	{
		if (value)
			stream.write(value, charTraits::length(value));
		else
			stream.write("(null)"_s);
	}
	void print(const char value) noexcept { stream.write(value); }

	template<typename T> void print(const std::unique_ptr<T []> &value) { print(value.get()); }
	template<typename T> typename std::enable_if<std::is_base_of<printable_t, T>::value>::type
		print(const T &printable) noexcept { printable(stream); }
	template<typename T> typename std::enable_if<isScalar<T>::value>::type
		print(const T &num) noexcept { write(asInt_t<T>{num}); }
	template<typename T> typename std::enable_if<!isChar<T>::value>::type
		print(const T *ptr) noexcept { write("0x", asHex_t<8, '0'>{reinterpret_cast<intptr_t>(ptr)}); }

	void print(const bool value) noexcept
	{
		if (value)
			stream.write("true"_s);
		else
			stream.write("false"_s);
	}

	template<typename T, size_t N> void print(const std::array<T, N> &arr) noexcept
	{
		for (auto &elem : arr)
			write(asHex_t<sizeof(T) * 2, '0'>(elem));
	}

public:
	streamFormatter_t(stream_t &_stream) noexcept : stream{_stream} { }
	streamFormatter_t &write() noexcept { return *this; }

	template<typename T, typename... U> streamFormatter_t &write(T &&value, U &&... values) noexcept
	{
		print(value);
		return write(std::forward<U>(values)...);
	}

	streamFormatter_t() = delete;
	streamFormatter_t(const streamFormatter_t &) = delete;
	streamFormatter_t(streamFormatter_t &&) = delete;
	streamFormatter_t &operator =(const streamFormatter_t &) = delete;
	streamFormatter_t &operator =(streamFormatter_t &&) = delete;
};

#endif /*STREAM_FORMATTER__HXX*/

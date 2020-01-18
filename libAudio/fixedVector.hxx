#ifndef FIXED_VECTOR__HXX
#define FIXED_VECTOR__HXX

#include <memory>
#include <new>
#include <utility>

template<typename T> struct boundedIterator_t final
{
private:
	T *const data;
	size_t index;
	const size_t max;

	constexpr boundedIterator_t(T *const ptr, const size_t _index, const size_t _max) noexcept :
		data{ptr}, index{_index}, max{_max} { }

public:
	using value_type = T;
	using pointer = T *;
	using reference = T &;

	constexpr boundedIterator_t(T *const ptr, const size_t _max) noexcept : boundedIterator_t{ptr, 0, _max} { }
	boundedIterator_t(const boundedIterator_t &iter) noexcept : boundedIterator_t{iter.data, iter.index, iter.max} { }
	boundedIterator_t(boundedIterator_t &&iter) noexcept : boundedIterator_t{iter.data, iter.index, iter.max} { }

	reference operator *() const noexcept { return data[index]; }
	pointer operator ->() const noexcept { return data + index; }
	boundedIterator_t &operator ++() noexcept { if (index < max) ++index; return *this; }
	boundedIterator_t &operator ++(int) noexcept
		{ if (index < max) return boundedIterator_t(data, index++, max); return *this; }
	boundedIterator_t &operator --() noexcept { if (index > 0) --index; return *this; }
	boundedIterator_t &operator --(int) noexcept
		{ if (index > 0) return boundedIterator_t(data, index--, max); return *this; }
	reference operator [](const size_t n) const noexcept { return *(*this + n); }
	// I think the > max case must also be addressed..
	boundedIterator_t &operator +=(const size_t n) noexcept { if ((index + n) <= max) index += n; return *this; }
	boundedIterator_t &operator +(const size_t n) const noexcept { return boundedIterator_t{*this} += n; }
	// The supper complicated looking condition here checkes that the subtraction won't underflow
	// and go super-large, in a way where we don't have to care what size_t really means
	// I think the < 0 case must also be addressed..
	boundedIterator_t &operator -=(const size_t n) noexcept
		{ if (!((index - n) & (1 << ((sizeof(size_t) * 8) - 1)))) index -= n; return *this; }
	boundedIterator_t &operator -(const size_t n) const noexcept { return boundedIterator_t{*this} -= n; }

	bool operator ==(const boundedIterator_t &b) const noexcept
		{ return data == b.data && index == b.index && max == b.max; }
	bool operator !=(const boundedIterator_t &b) const noexcept { return !(*this == b); }

	boundedIterator_t operator =(const boundedIterator_t &) noexcept = delete;
	boundedIterator_t operator =(boundedIterator_t &&) noexcept = delete;
};

struct vectorStateException_t final : public std::exception
{
	const char *what() const noexcept final override
		{ return "fixedVector_t in invalid state"; }
};

template<typename T> struct fixedVector_t final
{
private:
	std::unique_ptr<T []> _data;
	size_t _length;

public:
	using value_type = T;
	using reference = T &;
	using const_reference = const T &;
	using pointer = T *;
	using const_pointer = const T *const;
	using iterator = boundedIterator_t<T>;
	using const_iterator = boundedIterator_t<const T>;

	constexpr fixedVector_t() noexcept : _data(), _length(0) { }
	fixedVector_t(const size_t length) noexcept : _data(length ? new (std::nothrow) T[length]() : nullptr),
		_length(length) { }
	fixedVector_t(fixedVector_t &&vec) noexcept : fixedVector_t() { swap(vec); }
	//~fixedVector_t() noexcept { }
	fixedVector_t &operator =(fixedVector_t &&vec) noexcept { swap(vec); return *this; }

	size_t length() const noexcept { return _length; }
	size_t size() const noexcept { return _length; }
	bool valid() const noexcept { return bool(_data); }

	reference operator [](const size_t index)
	{
		if (!valid())
			throw vectorStateException_t();
		else if (index < _length)
			return _data[index];
		throw std::out_of_range("Index into fixedVector_t out of bounds");
	}

	const_reference operator [](const size_t index) const
	{
		if (!valid())
			throw vectorStateException_t();
		else if (index < _length)
			return _data[index];
		throw std::out_of_range("Index into fixedVector_t out of bounds");
	}

	iterator begin() noexcept { return iterator{_data.get(), _length}; }
	const_iterator begin() const noexcept { return const_iterator{_data.get(), _length}; }
	iterator end() noexcept { return begin() += _length; }
	const_iterator end() const noexcept { return begin() += _length; }

	void swap(fixedVector_t &vec) noexcept
	{
		std::swap(_data, vec._data);
		std::swap(_length, vec._length);
	}

	fixedVector_t(const fixedVector_t &) = delete;
	fixedVector_t &operator =(const fixedVector_t &) = delete;
};

#endif /*FIXED_VECTOR__HXX*/

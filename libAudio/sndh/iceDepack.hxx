// SPDX-License-Identifier: BSD-3-Clause
#ifndef SNDH_ICE_DEPACK_HXX
#define SNDH_ICE_DEPACK_HXX

#include <memory>
#include <array>
#include <substrate/fd>
#include <substrate/fixed_vector>

using substrate::fd_t;
using substrate::fixedVector_t;

struct sndhDepacker_t final
{
private:
	fixedVector_t<char> _data{};
	size_t _offset{};

	bool depack(const fd_t &file) noexcept;

public:
	sndhDepacker_t(const fd_t &file);
	[[nodiscard]] bool valid() const noexcept { return _data.valid(); }

	size_t seek(const off_t offset, const int32_t whence) noexcept
	{
		const auto length{_data.size()};
		if (whence == SEEK_SET)
		{
			if (offset < 0 || static_cast<size_t>(offset) > length)
				return false;
			_offset = offset;
		}
		else if (whence == SEEK_CUR)
		{
			const off_t newOffset{static_cast<off_t>(_offset) + offset};
			if (static_cast<size_t>(newOffset) > length || newOffset < 0)
				return false;
			_offset += offset;
		}
		else if (whence == SEEK_END)
		{
			const off_t newOffset{static_cast<off_t>(length) - offset};
			if (static_cast<size_t>(newOffset) > length || newOffset < 0)
				return false;
			_offset = length - offset;
		}
		else
			return false;
		return true;
	}

	[[nodiscard]] bool head() noexcept { return seek(0, SEEK_SET) == 0; }
	[[nodiscard]] bool tail() noexcept { return seek(0, SEEK_END) == _data.size(); }

	[[nodiscard]] bool seekRel(const off_t offset) noexcept
	{
		const auto currentOffset{_offset};
		return seek(offset, SEEK_CUR) == currentOffset + offset;
	}

	[[nodiscard]] size_t length() const noexcept { return _data.length(); }

	[[nodiscard]] bool read(void *const value, const size_t valueLen) noexcept
	{
		if (_offset + valueLen > _data.size())
			return false;
		std::memcpy(value, _data.data() + _offset, valueLen);
		_offset += valueLen;
		return true;
	}

	template<typename T> bool read(T &value) noexcept
		{ return read(&value, sizeof(T)); }
	template<typename T> bool write(const T &value) noexcept
		{ return write(&value, sizeof(T)); }
	template<typename T> bool read(std::unique_ptr<T> &value) noexcept
		{ return read(value.get(), sizeof(T)); }
	template<typename T> bool read(const std::unique_ptr<T> &value) noexcept
		{ return read(value.get(), sizeof(T)); }
	template<typename T> bool write(const std::unique_ptr<T> &value) noexcept
		{ return write(value.get(), sizeof(T)); }
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays,hicpp-avoid-c-arrays)
	template<typename T> bool read(const std::unique_ptr<T []> &value, const size_t valueCount) noexcept
		{ return read(value.get(), sizeof(T) * valueCount); }
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays,hicpp-avoid-c-arrays)
	template<typename T> bool write(const std::unique_ptr<T []> &value, const size_t valueCount) noexcept
		{ return write(value.get(), sizeof(T) * valueCount); }
	template<typename T, size_t N> bool read(std::array<T, N> &value) noexcept
		{ return read(value.data(), sizeof(T) * N); }
	template<typename T, size_t N> bool write(const std::array<T, N> &value) noexcept
		{ return write(value.data(), sizeof(T) * N); }

	template<typename T> bool read(const fixedVector_t<T> &value) const noexcept
		{ return read(value.data(), sizeof(T) * value.size()); }
	template<typename T> bool write(const fixedVector_t<T> &value) const noexcept
		{ return write(value.data(), sizeof(T) * value.size()); }

	template<size_t length, typename T, size_t N> bool read(std::array<T, N> &value) noexcept
	{
		static_assert(length <= N, "Can't request to read more than the std::array<> length");
		return read(value.data(), sizeof(T) * length);
	}

	[[nodiscard]] bool readLE(uint16_t &value) noexcept
	{
		std::array<uint8_t, 2> data{};
		const bool result = read(data);
		value = uint16_t((uint16_t(data[1]) << 8U) | data[0]);
		return result;
	}

	[[nodiscard]] bool readLE(uint32_t &value) noexcept
	{
		std::array<uint8_t, 4> data{};
		const bool result = read(data);
		value = (uint32_t(data[3]) << 24U) | (uint32_t(data[2]) << 16U) |
			(uint32_t(data[1]) << 8U) | data[0];
		return result;
	}

	[[nodiscard]] bool readLE(uint64_t &value) noexcept
	{
		std::array<uint8_t, 8> data{};
		const bool result = read(data);
		value = (uint64_t(data[7]) << 56U) | (uint64_t(data[6]) << 48U) |
			(uint64_t(data[5]) << 40U) | (uint64_t(data[4]) << 32U) |
			(uint64_t(data[3]) << 24U) | (uint64_t(data[2]) << 16U) |
			(uint64_t(data[1]) << 8U) | data[0];
		return result;
	}

	template<typename T, typename = typename std::enable_if<
		std::is_integral<T>::value && !std::is_same<T, bool>::value &&
		std::is_signed<T>::value && sizeof(T) >= 2>::type
	>
	bool readLE(T &value) noexcept
	{
		typename std::make_unsigned<T>::type data{};
		const auto result = readLE(data);
		value = static_cast<T>(data);
		return result;
	}

	[[nodiscard]] bool readBE(uint16_t &value) noexcept
	{
		std::array<uint8_t, 2> data{};
		const bool result = read(data);
		value = uint16_t((uint16_t(data[0]) << 8U) | data[1]);
		return result;
	}

	[[nodiscard]] bool readBE(uint32_t &value) noexcept
	{
		std::array<uint8_t, 4> data{};
		const bool result = read(data);
		value = (uint32_t(data[0]) << 24U) | (uint32_t(data[1]) << 16U) |
			(uint32_t(data[2]) << 8U) | data[3];
		return result;
	}

	[[nodiscard]] bool readBE(uint64_t &value) noexcept
	{
		std::array<uint8_t, 8> data{};
		const bool result = read(data);
		value = (uint64_t(data[0]) << 56U) | (uint64_t(data[1]) << 48U) |
			(uint64_t(data[2]) << 40U) | (uint64_t(data[3]) << 32U) |
			(uint64_t(data[4]) << 24U) | (uint64_t(data[5]) << 16U) |
			(uint64_t(data[6]) << 8U) | data[7];
		return result;
	}

	template<typename T, typename = typename std::enable_if<
		std::is_integral<T>::value && !std::is_same<T, bool>::value &&
		std::is_signed<T>::value && sizeof(T) >= 2>::type
	>
	bool readBE(T &value) noexcept
	{
		typename std::make_unsigned<T>::type data{};
		const auto result = readBE(data);
		value = static_cast<T>(data);
		return result;
	}
};

#endif /*SNDH_ICE_DEPACK_HXX*/

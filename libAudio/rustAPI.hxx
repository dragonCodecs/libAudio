// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>
#ifndef RUST_API_HXX
#define RUST_API_HXX

#include <cstdint>
#include <memory>

struct stringSlice_t
{
	uint8_t *ptr;
	size_t len;

	[[nodiscard]] std::unique_ptr<char []> clone() const
	{
		auto result = std::make_unique<char []>(len + 1);
		std::memcpy(result.get(), ptr, len);
		result[len] = '\0';
		return result;
	}
};

template<typename T> struct slice_t
{
	T *ptr;
	size_t len;

	[[nodiscard]] bool isEmpty() const
	{
		return ptr == nullptr || len == 0U;
	}

	T &operator [](const size_t index) const
	{
		return ptr[index];
	}
};

#endif /*RUST_API_HXX*/

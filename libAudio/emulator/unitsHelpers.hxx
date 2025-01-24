// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_UNITS_HELPERS_HXX
#define EMULATOR_UNITS_HELPERS_HXX

#include <cstdint>

constexpr uint64_t operator ""_KiB(const unsigned long long value) noexcept
	{ return static_cast<uint64_t>(value) * 1024U; }
constexpr uint64_t operator ""_MiB(const unsigned long long value) noexcept
	{ return static_cast<uint64_t>(value) * 1048576U; }
constexpr uint64_t operator ""_GiB(const unsigned long long value) noexcept
	{ return static_cast<uint64_t>(value) * 1073741824U; }
constexpr uint64_t operator ""_TiB(const unsigned long long value) noexcept
	{ return static_cast<uint64_t>(value) * 1099511627776U; }
constexpr uint64_t operator ""_PiB(const unsigned long long value) noexcept
	{ return static_cast<uint64_t>(value) * 1125899906842624U; }

constexpr uint64_t operator ""_kB(const unsigned long long value) noexcept
	{ return static_cast<uint64_t>(value) * 1000U; }
constexpr uint64_t operator ""_MB(const unsigned long long value) noexcept
	{ return static_cast<uint64_t>(value) * 1000000U; }
constexpr uint64_t operator ""_GB(const unsigned long long value) noexcept
	{ return static_cast<uint64_t>(value) * 1000000000U; }
constexpr uint64_t operator ""_TB(const unsigned long long value) noexcept
	{ return static_cast<uint64_t>(value) * 1000000000000U; }
constexpr uint64_t operator ""_PB(const unsigned long long value) noexcept
	{ return static_cast<uint64_t>(value) * 1000000000000000U; }

constexpr uint64_t operator ""_KHz(const unsigned long long value)
	{ return static_cast<uint64_t>(value) * 1000U; }
constexpr uint64_t operator ""_MHz(const unsigned long long value)
	{ return static_cast<uint64_t>(value) * 1000000U; }

#endif /*EMULATOR_UNITS_HELPERS_HXX*/

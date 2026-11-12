// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <cmath>
#include <random>
#include <numeric>
#include <substrate/span>
#include <substrate/utility>
#include <substrate/indexed_iterator>
#include "ym2149.hxx"
#include "rustAPI.hxx"

extern "C"
{
	Ym2149 *ym2149_new();
	void ym2149_drop(Ym2149 *);

	void ym2149_read_address(Ym2149 *, uint32_t,  slice_t<uint8_t>);
	void ym2149_write_address(const Ym2149 *, uint32_t, const slice_t<uint8_t>);
	bool ym2149_sample_ready(const Ym2149 *);
	int16_t ym2149_sample(Ym2149 *);
	bool ym2149_clock_cycle(Ym2149 *, ym2149_t *);
	void ym2149_force_edge_state(Ym2149 *, bool);

	bool ym2149_advance_cycle(ym2149_t *self)
	{
		return self->advanceCycle();
	}
}

ym2149_t::ym2149_t(const uint32_t clockFrequency, const uint32_t sampleFrequency) noexcept :
	clockedPeripheral_t<uint32_t>{clockFrequency}, clockManager{clockFrequency, sampleFrequency}
{
	self = ym2149_new();
}

ym2149_t::~ym2149_t() noexcept
{
	if (self)
		ym2149_drop(self);
}

void ym2149_t::readAddress(const uint32_t address, substrate::span<uint8_t> data) const noexcept
{
	ym2149_read_address(self, address, {data.data(), data.size_bytes()});
}

void ym2149_t::writeAddress(const uint32_t address, const substrate::span<uint8_t> &data) noexcept
{
	ym2149_write_address(self, address, {data.data(), data.size_bytes()});
}

bool ym2149_t::clockCycle() noexcept
{
	return ym2149_clock_cycle(self, this);
}

bool ym2149_t::advanceCycle() noexcept
{
	return clockManager.advanceCycle();
}

bool ym2149_t::sampleReady() const noexcept { return ym2149_sample_ready(self); }

namespace ym2149
{
	// FIXME: floating point function usage here is non-constexpr so can't be precomputed except on GCC!
#if 0
	namespace internal
	{
		// Logarithmic volume levels from 1 / (sqrt(2) ^ (level / 2))
		// The sequence is reversed though to get 0 to be the lowest volume
		[[nodiscard]] constexpr uint16_t logLevel(const uint8_t level) noexcept
		{
			// Do the main computation
			double result{1.0 / std::pow(std::sqrt(2.0), (31U - level) / 2.0)};
			// Scale the result to make space for 3 channels of max volume, and convert
			// to the range [0, 32767] to make the result
			return static_cast<uint16_t>((result / 3.0) * 32767U);
		}

		template<size_t N, uint16_t... table> struct calcLogTable
		{
			constexpr static std::array<uint16_t, sizeof...(table) + N + 1U> value
				{calcLogTable<N - 1U, logLevel(N), table...>::value};
		};

		template<uint16_t... table> struct calcLogTable<0U, table...>
		{
			constexpr static std::array<uint16_t, sizeof...(table) + 1U> value
				{{logLevel(0U), table...}};
		};
	} // namespace internal

	constexpr static auto logLevel{internal::calcLogTable<31U>::value};
#endif
} // namespace ym2149

int16_t ym2149_t::sample() noexcept
{
	return ym2149_sample(self);
}

void ym2149_t::forceChannelStates(const bool edgeState) noexcept
{
	ym2149_force_edge_state(self, edgeState);
}

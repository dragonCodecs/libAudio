// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#ifndef EMULATOR_SOUND_STEDAC_HXX
#define EMULATOR_SOUND_STEDAC_HXX

#include <cstdint>
#include <substrate/span>
#include "../memoryMap.hxx"
#include "../timing/mc68901.hxx"

namespace steDAC
{
	struct register24b_t final
	{
	private:
		uint32_t value{0U};

	public:
		void writeByte(uint8_t position, uint8_t byte) noexcept;
		[[nodiscard]] uint8_t readByte(uint8_t position) const noexcept;
		void reset() noexcept;

		[[nodiscard]] operator uint32_t() const noexcept { return value; }
		register24b_t &operator +=(uint32_t amount) noexcept;
	};
} // namespace steDAC

struct steDAC_t final : public clockedPeripheral_t<uint32_t>
{
private:
	void readAddress(uint32_t address, substrate::span<uint8_t> data) const noexcept final;
	void writeAddress(uint32_t address, const substrate::span<uint8_t> &data) noexcept final;

	mc68901_t &_mfp;

	// Values that control the DMA engine
	steDAC::register24b_t baseAddress{};
	steDAC::register24b_t endAddress{};
	steDAC::register24b_t sampleCounter{};
	uint8_t control{0U};
	bool sampleMono{true};
	uint8_t sampleRateDivider{0U};
	uint8_t sampleRateCounter{0U};

	// NB: only reason for these to be marked mutable is so microwireCycle() can be const
	// for use in the readAddress call - not ideal, but not completely terrible
	uint16_t microwireData{0U};
	mutable uint16_t microwireMask{0U};
	mutable uint8_t microwireCycles{0U};
	uint8_t mainVolume{64U};

	void runMicrowireTransaction() noexcept;
	[[nodiscard]] uint16_t microwireCycle() const noexcept;

public:
	steDAC_t(uint32_t clockFrequency, mc68901_t &mfp) noexcept;
	steDAC_t(const steDAC_t &) noexcept = delete;
	steDAC_t(steDAC_t &&) noexcept = delete;
	steDAC_t &operator =(const steDAC_t &) noexcept = delete;
	steDAC_t &operator =(steDAC_t &&) noexcept = delete;
	~steDAC_t() noexcept final = default;

	[[nodiscard]] bool clockCycle() noexcept final;
	[[nodiscard]] uint8_t outputLevel() const noexcept { return mainVolume; }
	[[nodiscard]] int16_t sample(const memoryMap_t<uint32_t, 0x00ffffffU> &memoryMap) const noexcept;
};

#endif /*EMULATOR_SOUND_STEDAC_HXX*/

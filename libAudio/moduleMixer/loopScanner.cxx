// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2010-2023 Rachel Mant <git@dragonmux.network>
#include <optional>
#include <substrate/span>
#include <substrate/indexed_iterator>
#include <substrate/index_sequence>
#include "../libAudio.hxx"
#include "../genericModule/genericModule.h"

/* If the row in the pattern has been visited */
#define ROW_VISITED 0x01U
/* If the reason that row has been visited is by pattern loop effect */
#define ROW_PATTERN_LOOPED 0x02U

struct patternState_t final
{
	fixedVector_t<uint8_t> rows{};
};

struct channelState_t final
{
	/* Pattern loop tracking */
	uint8_t patternLoopCount{};
	uint16_t patternLoopStart{};

	std::optional<uint16_t> patternLoop(uint8_t param, uint16_t row) noexcept;
};

struct scanState_t final
{
	/* Tracking for the states of each pattern */
	fixedVector_t<patternState_t> patterns;
	/* Tracking for the states of of the playback channels */
	fixedVector_t<channelState_t> channels;
	/* Span representing the pattern data to be used */
	substrate::span<pattern_t *> patternData;
	/* Span representing the order list to be used */
	substrate::span<uint8_t> orders;
	/* Track how many samples have been produced to this point in the tune - used to calcualte song length */
	uint64_t samplesProduced{};
	/* Current play speed tracking */
	uint32_t speed;
	uint32_t tempo;
	/* Tracking for the mix frequency being used to get accurate samples tracking */
	uint32_t sampleRate;
	uint32_t samplesPerTick{};
	/* Tick and delay tracking */
	uint32_t tickCount;
	uint8_t patternDelay{};
	uint8_t frameDelay{};
	/* Pattern and order tracking */
	uint16_t nextOrder{};
	uint16_t currentOrder{};
	uint16_t currentPattern{};

	/* Row tracking */
	uint16_t nextRow{};
	uint16_t currentRow{};
	uint16_t rowsInPattern{};

	scanState_t(size_t patternCount, size_t channelCount, substrate::span<pattern_t *>patternList,
		substrate::span<uint8_t> orderList, uint32_t musicSpeed, uint32_t musicTempo, uint32_t mixSampleRate);
	void updateSamplesPerTick() noexcept;
	void scan();
	bool tick();
	void processEffects() noexcept;
	void handleNavigationEffects(std::optional<uint16_t> patternLoopRow, std::optional<uint16_t> breakRow,
		std::optional<uint8_t> positionJump) noexcept;
	void disableJumpEffect() noexcept;
};

void ModuleFile::loopScanPatterns(fileInfo_t &info)
{
	/* State tracker for the scan */
	scanState_t state{p_Header->nPatterns, p_Header->nChannels, {p_Patterns, p_Header->nPatterns},
		{p_Header->Orders.get(), p_Header->nOrders}, MusicSpeed, MusicTempo, info.bitRate()};
	/* Ask the scanner to do its job across the patterns in the order specified by the tune */
	state.scan();
	/* Extract the final number of samples produced and stuff it into the info, converting to seconds */
	const uint64_t timeSeconds{state.samplesProduced / info.bitRate()};
	const bool timeRemainder{(state.samplesProduced % info.bitRate()) != 0U};
	info.totalTime(timeSeconds + (timeRemainder ? 1U : 0U));
}

scanState_t::scanState_t(const size_t patternCount, const size_t channelCount,
	const substrate::span<pattern_t *> patternList, const substrate::span<uint8_t> orderList,
	const uint32_t musicSpeed, const uint32_t musicTempo, const uint32_t mixSampleRate) : patterns{patternCount},
	channels{channelCount}, patternData{patternList}, orders{orderList}, speed{musicSpeed}, tempo{musicTempo},
	sampleRate{mixSampleRate}, tickCount{speed} { updateSamplesPerTick(); }

void scanState_t::updateSamplesPerTick() noexcept
	{ samplesPerTick = (sampleRate * 640U) / (tempo << 8U); }

void scanState_t::scan()
{
	/* Work our way through the tune, starting at the first pattern specified by the order list */
	while (tick() && tempo != 0U)
	{
		/* Process any effects the current row causes to fire that we care about */
		processEffects();
		/* Now make sure to update the sampling info */
		updateSamplesPerTick();
		samplesProduced += samplesPerTick;
	}
}

bool scanState_t::tick()
{
	/* If this tick will push us onto the next row */
	if (++tickCount >= (speed * (patternDelay + 1U)) + frameDelay)
	{
		/* Get ready for the next row's shenanigans, and set up for it */
		tickCount = 0;
		patternDelay = 0;
		frameDelay = 0;
		currentRow = nextRow;
		/* Check if we need to adjust which pattern we're processing */
		do
		{
			/* If this was the last pattern, we're done */
			if (nextOrder >= orders.size())
				return false;
			/* If the next selected pattern is not the next one from the orders, adjust where we go next */
			if (currentOrder != nextOrder)
				currentOrder = nextOrder;
			currentPattern = orders[currentOrder];
			/* If the selected pattern is beyond the last pattern, try the next one in the order list */
			if (currentPattern >= patterns.count())
				++nextOrder;
		}
		while (currentPattern >= patterns.count());
		nextOrder = currentOrder;
		/* Having adjusted the pattern we're on appropriately, now see if we need to adjust the row */
		if (currentRow >= rowsInPattern)
			currentRow = 0U;
		nextRow = currentRow + 1U;
		/* If at the end of this row, we'd end the current pattern, set up for the next pattern */
		if (nextRow >= rowsInPattern)
		{
			++nextOrder;
			nextRow = 0U;
		}
		/* If we hit a sentinel pattern, we're done */
		if (!patternData[currentPattern])
			return false;
		/*
		 * Otherwise, extract from the pattern how many rows there are, initialising the tracking for it
		 * if it's not already been
		 */
		const auto &pattern{*patternData[currentPattern]};
		rowsInPattern = pattern.rows();
		if (!patterns[currentPattern].rows.valid())
			patterns[currentPattern].rows = {rowsInPattern};
		/* Mark this row visited */
		patterns[currentPattern].rows[currentRow] |= ROW_VISITED;
	}
	if (speed == 0U)
		speed = 1U;
	return true;
}

void scanState_t::processEffects() noexcept
{
	std::optional<uint8_t> positionJump{};
	std::optional<uint16_t> breakRow{};
	std::optional<uint16_t> patternLoopRow{};
	const auto &pattern{*patternData[currentPattern]};
	/* Process the effects for each channel, focusing specifically on speed and position change effects */
	for (const auto &[idx, channel] : substrate::indexedIterator_t{channels})
	{
		/* If this channel is processing pattern loop effects, mark the row visited by pattern loop */
		if (channel.patternLoopStart && channel.patternLoopCount)
			patterns[currentPattern].rows[currentRow] |= ROW_PATTERN_LOOPED;
		const auto command{pattern.commands()[idx][currentRow]};
		const auto [effect, param]{command.effect()};
		switch (effect)
		{
			/* If the effect is either a MOD or S3M extended effect */
			case CMD_MOD_EXTENDED:
			case CMD_S3M_EXTENDED:
			{
				const auto extendedCommand{static_cast<uint8_t>((param & 0xf0U) >> 4U)};
				/* Only process extended commands on the first tick of a row */
				if (tickCount == 0U)
				{
					if ((effect == CMD_MOD_EXTENDED && extendedCommand == CMD_MODEX_LOOP) ||
						(effect == CMD_S3M_EXTENDED && extendedCommand == CMD_S3MEX_LOOP))
					{
						/* Try to handle the pattern loop */
						const auto loop{channel.patternLoop(param & 0x0fU, currentRow)};
						if (loop)
							patternLoopRow = loop;
					}
					/* Handle pattern delay commands */
					else if (effect == CMD_MODEX_DELAYPAT)
						patternDelay = param & 0x0fU;
				}
				break;
			}
			/* If the effect is a position jump, set up for the jump */
			case CMD_POSITIONJUMP:
				/* Adjust the jump destination to not go outside the number of orders available */
				if (param > orders.size())
					positionJump = {0};
				else
					positionJump = {param};
				break;
			/* If the effect is a pattern break, set up for the break destination */
			case CMD_PATTERNBREAK:
				/* Figure out if the new row position is outside the valid range, if so adjust */
				if (param >= rowsInPattern - 1U)
					breakRow = {rowsInPattern - 1U};
				else
					breakRow = {param};
				break;
			/* Track speed and tempo changes */
			case CMD_SPEED:
				speed = param;
				break;
			case CMD_TEMPO:
				tempo = param;
				break;
		}
	}
	/* Now we know where we're going and at what speed, process the nav effects */
	handleNavigationEffects(patternLoopRow, breakRow, positionJump);
}

void scanState_t::handleNavigationEffects(const std::optional<uint16_t> patternLoopRow,
	const std::optional<uint16_t> breakRow, const std::optional<uint8_t> positionJump) noexcept
{
	/* Only process on the first tick of the row */
	if (tickCount != 0U)
		return;
	/* If we have a pattern loop going, process that */
	if (patternLoopRow)
	{
		nextOrder = currentOrder;
		nextRow = *patternLoopRow;
		/* Adjust the next row to play by if there is a pattern delay in effect */
		if (patternDelay)
			++nextRow;
	}
	/* Otherwise, check to see if we need to handle an effect that jumps about */
	if (breakRow || positionJump)
	{
		/* Unpack where to jump to - if there is no valid position jump, it's the next selected pattern */
		const auto jumpOrder{positionJump.value_or(currentOrder + 1U)};
		/* Unpack what row to go to - if there's no valid row, it's the first of the new pattern */
		auto targetRow{breakRow.value_or(0U)};
		const auto pattern{orders[jumpOrder]};
		/*
		 * Check to see if we've already visited the jump target,
		 * starting by seeing if the target pattern's ever been run
		 */
		if (jumpOrder < orders.size() && patterns[pattern].rows)
		{
			/* Adjust the target row if it's outside the target pattern */
			if (targetRow >= patterns[pattern].rows.count())
				targetRow = 0U;
			/* As it has, see if we've ever jumped to the target row before then */
			if ((patterns[pattern].rows[targetRow] & ~ROW_PATTERN_LOOPED) == ROW_VISITED)
			{
				/* Don't take the jump, instead disable it */
				disableJumpEffect();
				return;
			}
		}
		/* It's a valid jump if we get here, so do it */
		if (jumpOrder != currentOrder || targetRow != currentRow)
		{
			/* Clear out any pattern loop state as we do this */
			if (jumpOrder != currentOrder)
			{
				for (auto &channel : channels)
				{
					channel.patternLoopCount = 0U;
					channel.patternLoopStart = 0U;
				}
			}
			/* And set up to hit the new pattern and row */
			nextOrder = jumpOrder;
			nextRow = targetRow;
		}
	}
}

void scanState_t::disableJumpEffect() noexcept
{
	const auto &pattern{*patternData[currentPattern]};
	/* Scan through all the channels' command data */
	for (const auto idx : substrate::indexSequence_t{channels.count()})
	{
		/* Extract the appropriate command and effect data */
		auto &command{pattern.commands()[idx][currentRow]};
		const auto [effect, param]{command.effect()};
		/* If this command is either a pattern jump or position jump, mark it disabled */
		if (effect == CMD_PATTERNBREAK || effect == CMD_POSITIONJUMP)
			command.disableEffect();
	}
}

std::optional<uint16_t> channelState_t::patternLoop(const uint8_t param, const uint16_t row) noexcept
{
	/* Figure out where the pattern loop is to */
	if (param)
	{
		/*
		 * If we're already processing a pattern loop, we just hit the command again,
		 * so check to see if there are any loops needed left
		 */
		if (patternLoopCount)
		{
			/* If this is the last loop */
			if (!--patternLoopCount)
			{
				// Reset the default start position for the next CMDEX_LOOP
				patternLoopStart = 0;
				return {};
			}
		}
		/* If there are no loops left to process, this is a new loop */
		else
			patternLoopCount = param;
		return {patternLoopStart};
	}
	/* If this was to set where the loop should start, store that */
	else
		patternLoopStart = row;
	return {};
}

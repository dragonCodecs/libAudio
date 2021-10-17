// SPDX-License-Identifier: BSD-3-Clause
#ifndef LIBAUDIO_MODULEMIXER_MIXFUNCTIONTABLES_H
#define LIBAUDIO_MODULEMIXER_MIXFUNCTIONTABLES_H

#include "mixFunctions.h"

#define MIX_NOSRC		0x00
#define MIX_RAMP		0x01
#define MIX_LINEARSRC	0x02
#define MIX_HQSRC		0x04
#define MIX_FILTER		0x08
#define MIX_STEREO		0x10
#define MIX_16BIT		0x20

const std::array<MixInterface, 64> MixFunctionTable
{{
	//// 8-bit ////
	// Mono
	// Non filtering functions
	Mono8BitMix, Mono8BitRampMix, Mono8BitLinearMix, Mono8BitLinearRampMix,
	Mono8BitHQMix, Mono8BitHQRampMix, NULL, NULL,
	// Filtering functions
	FilterMono8BitMix, FilterMono8BitRampMix, FilterMono8BitLinearMix, FilterMono8BitLinearRampMix,
	FilterMono8BitHQMix, FilterMono8BitHQRampMix, NULL, NULL,
	// Stereo
	// Non filtering functions
	Stereo8BitMix, Stereo8BitRampMix, NULL, NULL,
	NULL, NULL, NULL, NULL,
	// Filtering functions
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	//// 16-bit ////
	// Non filtering functions
	Mono16BitMix, Mono16BitRampMix, Mono16BitLinearMix, Mono16BitLinearRampMix,
	Mono16BitHQMix, Mono16BitHQRampMix, NULL, NULL,
	// Filtering functions
	FilterMono16BitMix, FilterMono16BitRampMix, FilterMono16BitLinearMix, FilterMono16BitLinearRampMix,
	FilterMono16BitHQMix, FilterMono16BitHQRampMix, NULL, NULL,
	// Stereo
	// Non filtering functions
	Stereo16BitMix, Stereo16BitRampMix, NULL, NULL,
	NULL, NULL, NULL, NULL,
	// Filtering functions
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
}};

#endif /*LIBAUDIO_MODULEMIXER_MIXFUNCTIONTABLES_H*/

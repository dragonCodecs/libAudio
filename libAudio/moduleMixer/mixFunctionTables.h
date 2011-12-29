#ifndef __mixFunctionTables_H__
#define __mixFunctionTables_H__

#define MIX_NOSRC			0x00
#define MIX_RAMP			0x01
#define MIX_LINEARSRC		0x02
#define MIX_HQSRC			0x04
#define MIX_KAISERSRC		0x06
#define MIX_FIRFILTERSRC	0x08
#define MIX_FILTER			0x10
#define MIX_16BIT			0x20

MixInterface MixFunctionTable[64] = 
{
	//// 8-bit ////
	// Non filtering functions
	Mono8BitMix, Mono8BitRampMix, Mono8BitLinearMix, Mono8BitLinearRampMix,
	Mono8BitHQMix, Mono8BitHQRampMix, Mono8BitKaiserMix, Mono8BitKaiserRampMix,
	Mono8BitFIRFilterMix, Mono8BitFIRFilterRampMix, NULL, NULL,
	NULL, NULL, NULL, NULL,
	// Filtering functions
	FilterMono8BitMix, FilterMono8BitRampMix, FilterMono8BitLinearMix, FilterMono8BitLinearRampMix,
	FilterMono8BitHQRampMix, FilterMono8BitLinearRampMix, FilterMono8BitKaiserRampMix, FilterMono8BitKaiserRampMix,
	FilterMono8BitFIRFilterRampMix, FilterMono8BitFIRFilterRampMix, NULL, NULL,
	NULL, NULL, NULL, NULL,
	//// 16-bit ////
	// Non filtering functions
	Mono16BitMix, Mono16BitRampMix, Mono16BitLinearMix, Mono16BitLinearRampMix,
	Mono16BitHQMix, Mono16BitHQRampMix, Mono16BitKaiserMix, Mono16BitKaiserRampMix,
	Mono16BitFIRFilterMix, Mono16BitFIRFilterRampMix, NULL, NULL,
	NULL, NULL, NULL, NULL,
	// Filtering functions
	FilterMono16BitMix, FilterMono16BitRampMix, FilterMono16BitLinearMix, FilterMono16BitLinearRampMix,
	FilterMono16BitHQRampMix, FilterMono16BitLinearRampMix, FilterMono16BitKaiserRampMix, FilterMono16BitKaiserRampMix,
	FilterMono16BitFIRFilterRampMix, FilterMono16BitFIRFilterRampMix, NULL, NULL,
	NULL, NULL, NULL, NULL,
};

#endif /*__mixFunctionTables_H__*/


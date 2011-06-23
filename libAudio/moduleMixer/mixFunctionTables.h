#ifndef __mixFunctionTables_H__
#define __mixFunctionTables_H__

#define MIX_NOSRC			0x00
#define MIX_RAMP			0x01
#define MIX_LINEARSRC		0x02
#define MIX_HQSRC			0x04
#define MIX_KAISERSRC		0x06
#define MIX_FIRFILTERSRC	0x08
#define MIX_FILTER			0x10

MixInterface MixFunctionTable[32] = 
{
	// Non ramping functions
	Mono8BitMix, Mono8BitRampMix, Mono8BitLinearMix, Mono8BitLinearRampMix,
	Mono8BitHQMix, Mono8BitLinearMix, Mono8BitKaiserMix, Mono8BitKaiserMix,
	Mono8BitFIRFilterMix, Mono8BitFIRFilterMix, NULL, NULL,
	NULL, NULL, NULL, NULL,
	// Ramping functions
	FilterMono8BitMix, FilterMono8BitRampMix, FilterMono8BitLinearMix, FilterMono8BitLinearRampMix,
	FilterMono8BitHQRampMix, FilterMono8BitLinearRampMix, FilterMono8BitKaiserRampMix, FilterMono8BitKaiserRampMix,
	FilterMono8BitFIRFilterRampMix, FilterMono8BitFIRFilterRampMix, NULL, NULL,
	NULL, NULL, NULL, NULL,
};

#endif /*__mixFunctionTables_H__*/


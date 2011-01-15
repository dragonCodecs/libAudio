#if !defined(COMMONH)
#define COMMONH

#include <mmsystem.h>

//===========================================================================
// Result from creating a new decoder object
//===========================================================================
enum ENOpenResult {OR_OK, OR_CODECERROR, OR_FileError, OR_MemError, OR_NoRights};
enum ENDecodeResult {DR_OK, DR_CODECERROR, DR_FileError, DR_MemError};

//===========================================================================
// Results from the sound output object
//===========================================================================
enum SOOpenResult {SO_OK, SO_MemError, SO_DEVICEOUTERR};

//---Graphic Eq---
#define GraphicEqFFTPoints 512			// At 44100Hz the lowest freq is   1 / (512 / 44100) = 86Hz   pos 1 = 86  pos 2 = 172
#define SOBuffersAddForEQ (GraphicEqFFTPoints * 4)	// *4 for 16 bit stereo

#endif
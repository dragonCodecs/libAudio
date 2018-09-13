#include "../libAudio.h"
#include "../libAudio_Common.h"
#include "genericModule.h"

#ifndef _WINDOWS
#ifndef max
#define max(a, b) (a > b ? a : b)
#endif
#endif

moduleFile_t::moduleFile_t(audioType_t type, fd_t &&fd) noexcept : audioFile_t(type, std::move(fd)), ctx(makeUnique<decoderContext_t>()) { }

void *ModuleAllocator::operator new(const size_t size)
{
	void *ret = ::operator new(size);
	memset(ret, 0x00, size);
	return ret;
}

void *ModuleAllocator::operator new[](const size_t size)
{
	void *ret = ::operator new[](size);
	memset(ret, 0x00, size);
	return ret;
}

void *ModuleAllocator::operator new(const size_t size, const std::nothrow_t &)
{
	void *ret = ::operator new(size, std::nothrow);
	memset(ret, 0x00, size);
	return ret;
}

void *ModuleAllocator::operator new[](const size_t size, const std::nothrow_t &)
{
	void *ret = ::operator new[](size, std::nothrow);
	memset(ret, 0x00, size);
	return ret;
}

ModuleFile::ModuleFile(const modMOD_t &file) : ModuleType(MODULE_MOD), p_Instruments(nullptr), Channels(nullptr), MixerChannels(nullptr)
{
	uint32_t i, maxPattern;
	const fd_t &fd = file.fd();

	p_Header = new ModuleHeader(file);
	if (fd.seek(20, SEEK_SET) != 20)
		throw ModuleLoaderError(E_BAD_MOD);
	p_Samples = new ModuleSample *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
		p_Samples[i] = ModuleSample::LoadSample(file, i);
	fd.seek(130 + (p_Header->nSamples != 15 ? 4 : 0), SEEK_CUR);

	// Count the number of patterns present
	for (i = 0, maxPattern = 0; i < p_Header->nOrders; i++)
	{
		if (p_Header->Orders[i] < 128)
			maxPattern = max(maxPattern, p_Header->Orders[i]);
	}
	p_Header->nPatterns = maxPattern + 1;
	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	for (i = 0; i < p_Header->nPatterns; i++)
		p_Patterns[i] = new ModulePattern(file, p_Header->nChannels);

	modLoadPCM(file);
	MinPeriod = 56;
	MaxPeriod = 7040;
}

ModuleFile::ModuleFile(const modS3M_t &file) : ModuleType(MODULE_S3M), p_Instruments(nullptr), Channels(nullptr), MixerChannels(nullptr)
{
	uint16_t i;
	const fd_t &fd = file.fd();

	p_Header = new ModuleHeader(file);
	p_Samples = new ModuleSample *[p_Header->nSamples];
	uint16_t *const SamplePtrs = p_Header->SamplePtrs.get<uint16_t>();
	for (i = 0; i < p_Header->nSamples; ++i)
	{
		const uint32_t offset = uint32_t{SamplePtrs[i]} << 4;
		if (fd.seek(offset, SEEK_SET) != offset)
			throw ModuleLoaderError(E_BAD_S3M);
		p_Samples[i] = ModuleSample::LoadSample(file, i);
	}

	// Count the number of channels present
	p_Header->nChannels = 32;
	for (i = 0; i < 32; ++i)
	{
		if (p_Header->ChannelSettings[i] & 0x80)
		{
			p_Header->nChannels = i;
			break;
		}
	}

	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	uint16_t *const PatternPtrs = p_Header->PatternPtrs.get<uint16_t>();
	for (i = 0; i < p_Header->nPatterns; ++i)
	{
		const uint32_t offset = uint32_t{PatternPtrs[i]} << 4;
		if (fd.seek(offset, SEEK_SET) != offset)
			throw ModuleLoaderError(E_BAD_S3M);
		p_Patterns[i] = new ModulePattern(file, p_Header->nChannels);
	}

	s3mLoadPCM(file);
	MinPeriod = 64;
	MaxPeriod = 32767;
}

ModuleFile::ModuleFile(STM_Intern *p_SF) : ModuleType(MODULE_STM), p_Instruments(nullptr), Channels(nullptr), MixerChannels(nullptr)
{
	uint32_t i;
	const fd_t &fd = p_SF->inner.fd();
	FILE *f_STM = p_SF->f_Module;

	p_Header = new ModuleHeader(p_SF->inner);
	p_Samples = new ModuleSample *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
		p_Samples[i] = ModuleSample::LoadSample(p_SF->inner, i);
	fd.seek(128, SEEK_CUR);
	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	for (i = 0; i < p_Header->nPatterns; i++)
		p_Patterns[i] = new ModulePattern(p_SF->inner);
	fseek(f_STM, 1104 + (1024 * p_Header->nPatterns), SEEK_SET);

	STMLoadPCM(f_STM);
	MinPeriod = 64;
	MaxPeriod = 32767;
}

// http://www.tigernt.com/onlineDoc/68000.pdf
// http://eab.abime.net/showthread.php?t=21516
// ftp://ftp.modland.com/pub/documents/format_documentation/Art%20Of%20Noise%20(.aon).txt
ModuleFile::ModuleFile(AON_Intern *p_AF) : ModuleType(MODULE_AON), p_Instruments(nullptr), Channels(nullptr), MixerChannels(nullptr)
{
	char StrMagic[4];
	uint32_t BlockLen, i, SampleLengths, InstrPos, PCMPos;
	uint8_t ChannelMul;
	FILE *f_AON = p_AF->f_Module;

	p_Header = new ModuleHeader(p_AF);

	fread(StrMagic, 4, 1, f_AON);
	fread(&BlockLen, 4, 1, f_AON);
	BlockLen = Swap32(BlockLen);
	// 2 if 8 voices, 1 otherwise
	ChannelMul = p_Header->nChannels >> 2;
	// Transform that into a shift value to get the number of patterns
	ChannelMul += 9;
	if (strncmp(StrMagic, "PATT", 4) != 0 || (BlockLen % (1 << ChannelMul)) != 0)
		throw new ModuleLoaderError(E_BAD_AON);
	p_Header->nPatterns = BlockLen >> ChannelMul;
	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	for (i = 0; i < p_Header->nPatterns; i++)
		p_Patterns[i] = new ModulePattern(p_AF, p_Header->nChannels);

	fread(StrMagic, 4, 1, f_AON);
	fread(&BlockLen, 4, 1, f_AON);
	BlockLen = Swap32(BlockLen);
	if (strncmp(StrMagic, "INST", 4) != 0 || (BlockLen % 32) != 0)
		throw new ModuleLoaderError(E_BAD_AON);
	p_Header->nSamples = BlockLen >> 5;
	InstrPos = ftell(f_AON);
	fseek(f_AON, BlockLen, SEEK_CUR);

	fread(StrMagic, 4, 1, f_AON);
	fread(&BlockLen, 4, 1, f_AON);
	BlockLen = Swap32(BlockLen);
	if (strncmp(StrMagic, "INAM", 4) == 0)
	{
		// We don't care about the instrument names, so skip over them.
		fseek(f_AON, BlockLen, SEEK_CUR);
		fread(StrMagic, 4, 1, f_AON);
		fread(&BlockLen, 4, 1, f_AON);
		BlockLen = Swap32(BlockLen);
	}
	if (strncmp(StrMagic, "WLEN", 4) != 0 || BlockLen != 0x0100)
		throw new ModuleLoaderError(E_BAD_AON);
	LengthPCM = new uint32_t[64];
	for (i = 0, SampleLengths = 0; i < 64; i++)
	{
		fread(LengthPCM + i, 4, 1, f_AON);
		LengthPCM[i] = Swap32(LengthPCM[i]);
		SampleLengths += LengthPCM[i];
		if (LengthPCM[i] != 0)
			nPCM = i + 1;
	}
	PCMPos = ftell(f_AON);

	p_Samples = new ModuleSample *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
	{
		fseek(f_AON, InstrPos + (i << 5), SEEK_SET);
		p_Samples[i] = ModuleSample::LoadSample(p_AF, i, nullptr, LengthPCM);
	}

	fseek(f_AON, PCMPos, SEEK_SET);
	fread(StrMagic, 4, 1, f_AON);
	fread(&BlockLen, 4, 1, f_AON);
	BlockLen = Swap32(BlockLen);
	if (strncmp(StrMagic, "WAVE", 4) != 0 || BlockLen != SampleLengths)
		throw new ModuleLoaderError(E_BAD_AON);

	AONLoadPCM(f_AON);
	MinPeriod = 56;
	MaxPeriod = 7040;
}

ModuleFile::ModuleFile(FC1x_Intern *p_FF) : ModuleType(MODULE_FC1x), p_Instruments(nullptr), Channels(nullptr), MixerChannels(nullptr)
{
#ifdef __FC1x_EXPERIMENTAL__
//	FILE *f_FC1x = p_FF->f_Module;

	p_Header = new ModuleHeader(p_FF);
#endif
}

ModuleFile::ModuleFile(const modIT_t &file) : ModuleType(MODULE_IT), p_Instruments(), Channels(), MixerChannels()
{
	const fd_t &fd = file.fd();
	uint16_t i;

	p_Header = new ModuleHeader(file);
	if (p_Header->nInstruments)
	{
		p_Instruments = new ModuleInstrument *[p_Header->nInstruments];
		uint32_t *const instrOffsets = p_Header->InstrumentPtrs.get<uint32_t>();
		for (i = 0; i < p_Header->nInstruments; ++i)
		{
			if (fd.seek(instrOffsets[i], SEEK_SET) != instrOffsets[i])
				throw ModuleLoaderError(E_BAD_IT);
			p_Instruments[i] = ModuleInstrument::LoadInstrument(file, i, p_Header->FormatVersion);
		}
	}
	p_Samples = new ModuleSample *[p_Header->nSamples];
	uint32_t *const sampleOffsets = p_Header->SamplePtrs.get<uint32_t>();
	for (i = 0; i < p_Header->nSamples; ++i)
	{
		if (fd.seek(sampleOffsets[i], SEEK_SET) != sampleOffsets[i])
			throw ModuleLoaderError(E_BAD_IT);
		p_Samples[i] = ModuleSample::LoadSample(file, i);
	}

	p_Header->nChannels = 64;
	for (i = 0; i < 64; i++)
	{
		p_Header->PanSurround[i] = false;
		if (p_Header->Panning[i] > 128)
		{
			p_Header->nChannels = i;
			break;
		}
		else if (p_Header->Panning[i] <= 64)
			p_Header->Panning[i] <<= 2;
		else if (p_Header->Panning[i] == 100)
		{
			p_Header->Panning[i] = 128;
			p_Header->PanSurround[i] = true;
		}
	}

	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	uint32_t *const PatternPtrs = p_Header->PatternPtrs.get<uint32_t>();
	for (i = 0; i < p_Header->nPatterns; i++)
	{
		if (PatternPtrs[i] == 0)
			p_Patterns[i] = nullptr;
		else
		{
			if (fd.seek(PatternPtrs[i], SEEK_SET) != PatternPtrs[i])
				throw ModuleLoaderError(E_BAD_IT);
			p_Patterns[i] = new ModulePattern(file, p_Header->nChannels);
		}
	}

	itLoadPCM(file);
	MinPeriod = 8;
	MaxPeriod = 61440;//32767;
}

ModuleFile::~ModuleFile()
{
	uint32_t i;

	DeinitMixer();

	delete [] LengthPCM;
	if (ModuleType != MODULE_AON)
		nPCM = p_Header->nSamples;
	for (i = 0; i < nPCM; i++)
		delete [] p_PCM[i];
	delete [] p_PCM;
	for (i = 0; i < p_Header->nPatterns; i++)
		delete p_Patterns[i];
	delete [] p_Patterns;
	for (i = 0; i < p_Header->nInstruments; i++)
		delete p_Instruments[i];
	delete [] p_Instruments;
	for (i = 0; i < p_Header->nSamples; i++)
		delete p_Samples[i];
	delete [] p_Samples;
	delete p_Header;
}

stringPtr_t ModuleFile::title() const noexcept
	{ return p_Header ? stringDup(p_Header->Name) : nullptr; }

stringPtr_t ModuleFile::author() const noexcept
	{ return p_Header ? stringDup(p_Header->Author) : nullptr; }

stringPtr_t ModuleFile::remark() const noexcept
	{ return p_Header ? stringDup(p_Header->Remark) : nullptr; }

uint8_t ModuleFile::channels() const noexcept
{
	if (!p_Header)
		return 0;
	return (p_Header->MasterVolume & 0x80) ? 2 : 1;
}

void ModuleFile::modLoadPCM(const modMOD_t &file)
{
	const fd_t &fd = file.fd();
	p_PCM = new uint8_t *[p_Header->nSamples];
	for (uint32_t i = 0; i < p_Header->nSamples; ++i)
	{
		uint32_t Length = p_Samples[i]->GetLength();
		if (Length != 0)
		{
			p_PCM[i] = new uint8_t[Length];
			if (!fd.read(p_PCM[i], Length))
				throw ModuleLoaderError(E_BAD_MOD);
			// TODO: This is hard, ok? MPT does wierd stuff here on memory buffers.
			// The decompression loop is now correct, as is the memory allocation.
			// However, I do not think the seek is correct, nor some of the length compensation code.
			// There really is no nice way either to express the sample's real length yet..
			// not sure that rewriting it is even correct, seeing we over-read (according to MPT), not under.
			/*if (strncasecmp(reinterpret_cast<char *>(p_PCM[i]), "ADPCM", 5) == 0)
			{
				const std::unique_ptr<uint8_t []> _(p_PCM[i]);
				const uint8_t *const compressionTable = _.get() + 5;
				const uint8_t *const compBuffer = _.get() + 5 + 16;
				uint8_t delta = 0;
				Length -= 16 + 5 - 1;
				Length &= ~1;
				p_PCM = new uint8_t[Length];
				p_Samples[i]->Length = Length;
				Length >>= 1;
				for (uint32_t j = 0, k = 0; j < Length; ++j)
				{
					delta += compressionTable[compBuffer[j] & 0x0F];
					p_PCM[i][k++] = delta;
					delta += compressionTable[(compBuffer[j] >> 4) & 0x0F];
					p_PCM[i][k++] = delta;
				}
				fseek(f_MOD, -Length, SEEK_CUR);
			}*/
			p_PCM[i][0] = p_PCM[i][1] = 0;
		}
		else
			p_PCM[i] = nullptr;
	}
}

void ModuleFile::s3mLoadPCM(const modS3M_t &file)
{
	const fd_t &fd = file.fd();
	p_PCM = new uint8_t *[p_Header->nSamples];
	for (uint32_t i = 0; i < p_Header->nSamples; ++i)
	{
		const uint32_t length = p_Samples[i]->GetLength() << (p_Samples[i]->Get16Bit() ? 1 : 0);
		if (length != 0 && p_Samples[i]->GetType() == 1)
		{
			const auto *sample = reinterpret_cast<ModuleSampleNative *>(p_Samples[i]);
			const uint32_t offset = uint32_t{sample->SamplePos} << 4;
			p_PCM[i] = new uint8_t[length];
			if (fd.seek(offset, SEEK_SET) != offset ||
				!fd.read(p_PCM[i], length))
				throw ModuleLoaderError(E_BAD_S3M);
			if (p_Header->FormatVersion == 2)
			{
				if (p_Samples[i]->Get16Bit())
				{
					auto *pcm = reinterpret_cast<uint16_t *>(p_PCM[i]);
					for (uint32_t j = 0; j < (length >> 1); j++)
						pcm[j] ^= 0x8000;
				}
				else
				{
					auto *pcm = reinterpret_cast<uint8_t *>(p_PCM[i]);
					for (uint32_t j = 0; j < length; j++)
						pcm[j] ^= 0x80;
				}
			}
		}
		else
			p_PCM[i] = nullptr;
	}
}

void ModuleFile::STMLoadPCM(FILE *f_STM)
{
	uint32_t i;
	p_PCM = new uint8_t *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
	{
		uint32_t Length = p_Samples[i]->GetLength();
		if (Length != 0)
		{
			p_PCM[i] = new uint8_t[Length];
			fread(p_PCM[i], Length, 1, f_STM);
			fseek(f_STM, Length % 16, SEEK_CUR);
		}
		else
			p_PCM[i] = nullptr;
	}
}

void ModuleFile::AONLoadPCM(FILE *f_AON)
{
	uint32_t i;
	p_PCM = new uint8_t *[nPCM];
	for (i = 0; i < nPCM; i++)
	{
		uint32_t Length = LengthPCM[i];
		if (Length != 0)
		{
			p_PCM[i] = new uint8_t[Length];
			fread(p_PCM[i], Length, 1, f_AON);
		}
		else
			p_PCM[i] = nullptr;
	}
}

uint32_t itBitstreamRead(uint8_t &buff, uint8_t &buffLen, const fd_t &fd, uint8_t bits)
{
	uint32_t ret = 0;
	if (bits > 0)
	{
		for (uint8_t i = 0; i < bits; i++)
		{
			if (buffLen == 0)
			{
				fd.read(buff);
				buffLen = 8;
			}
			ret |= (buff & 1) << i;
			buff >>= 1;
			buffLen--;
		}
	}
	return ret;
}

void itUnpackPCM8(ModuleSample *sample, uint8_t *PCM, const fd_t &fd, bool deltaComp)
{
	uint8_t buff, buffLen, bitWidth = 9;
	int8_t delta = 0, adjDelta = 0;
	uint32_t blockLen = 0, i = 0, Length;

	Length = sample->GetLength();
	while (Length != 0)
	{
		uint32_t j, offs = 0;
		if (blockLen == 0)
		{
			blockLen = 0x8000;
			buffLen = 0;
			// First we ignore 16 bits..
			itBitstreamRead(buff, buffLen, fd, 16);
			bitWidth = 9;
			delta = 0;
			adjDelta = 0;
		}
		j = blockLen;
		if (j > Length)
			j = Length;
		do
		{
			uint16_t bits;
			bits = itBitstreamRead(buff, buffLen, fd, bitWidth);
			if (fd.isEOF())
				return;
			if (bitWidth < 7)
			{
				uint16_t special = 1 << (bitWidth - 1);
				if (bits == special)
				{
					uint8_t bits = itBitstreamRead(buff, buffLen, fd, 3) + 1;
					if (bits < bitWidth)
						bitWidth = bits;
					else
						bitWidth = bits + 1;
					continue;
				}
			}
			else if (bitWidth < 9)
			{
				uint16_t special1 = (0xFF >> (9 - bitWidth)) + 4;
				uint16_t special2 = special1 - 8;
				if (bits > special2 && bits <= special1)
				{
					bits -= special2;
					if ((bits & 0xFF) < bitWidth)
						bitWidth = bits;
					else
						bitWidth = bits + 1;
					continue;
				}
			}
			else if (bitWidth > 9)
			{
				offs++;
				continue;
			}
			else if (bits >= 256)
			{
				bitWidth = bits + 1;
				continue;
			}
			if (bitWidth < 8)
			{
				uint8_t shift = 8 - bitWidth;
				bits = ((int8_t)(bits << shift)) >> shift;
			}
			delta += bits;
			adjDelta += delta;
			if (offs >= Length)
				return;
			PCM[i + offs] = deltaComp ? adjDelta : delta;
			offs++;
		}
		while (offs < j);
		i += j;
		Length -= j;
		blockLen -= j;
	}
}

void itUnpackPCM16(ModuleSample *sample, uint16_t *PCM, const fd_t &fd, bool deltaComp)
{
	uint8_t buff, buffLen, bitWidth = 17;
	int16_t delta = 0, adjDelta = 0;
	uint32_t blockLen = 0, i = 0, Length;

	Length = sample->GetLength() >> 1;
	while (Length != 0)
	{
		uint32_t j, offs = 0;
		if (blockLen == 0)
		{
			blockLen = 0x4000;
			buffLen = 0;
			// First we ignore 16 bits
			itBitstreamRead(buff, buffLen, fd, 16);
			bitWidth = 17;
			delta = 0;
			adjDelta = 0;
		}
		j = blockLen;
		if (j > Length)
			j = Length;
		do
		{
			uint32_t bits;
			if (fd.isEOF())
				return;
			bits = itBitstreamRead(buff, buffLen, fd, bitWidth);
			if (bitWidth < 7)
			{
				uint32_t special = 1 << (bitWidth - 1);
				if (bits == special)
				{
					uint8_t bits = itBitstreamRead(buff, buffLen, fd, 4) + 1;
					if (bits < bitWidth)
						bitWidth = bits;
					else
						bitWidth = bits + 1;
					continue;
				}
			}
			else if (bitWidth < 17)
			{
				uint16_t special1 = (0xFFFF >> (17 - bitWidth)) + 8;
				uint16_t special2 = special1 - 16;
				if (bits > special2 && bits <= special1)
				{
					bits -= special2;
					if ((bits & 0xFF) < bitWidth)
						bitWidth = bits;
					else
						bitWidth = bits + 1;
					continue;
				}
			}
			else if (bitWidth > 17)
			{
				offs++;
				continue;
			}
			else if (bits >= 65536)
			{
				bitWidth = bits + 1;
				continue;
			}
			if (bitWidth < 16)
			{
				uint8_t shift = 16 - bitWidth;
				bits = ((int16_t)(bits << shift)) >> shift;
			}
			delta += bits;
			adjDelta += delta;
			if (offs >= Length)
				return;
			PCM[i + offs] = deltaComp ? adjDelta : delta;
			offs++;
		}
		while (offs < j);
		i += j;
		Length -= j;
		blockLen -= j;
	}
}

template<typename T>
void stereoInterleave(T *pcmIn, T *pcmOut, uint32_t Length)
{
	for (uint32_t i = 0; i < Length; i++)
	{
		pcmOut[(i << 1) + 0] = pcmIn[i];
		pcmOut[(i << 1) + 1] = pcmIn[i + Length];
	}
}

void ModuleFile::itLoadPCM(const modIT_t &file)
{
	const fd_t &fd = file.fd();
	p_PCM = new uint8_t *[p_Header->nSamples];
	for (uint32_t i = 0; i < p_Header->nSamples; ++i)
	{
		ModuleSampleNative *const Sample = reinterpret_cast<ModuleSampleNative *>(p_Samples[i]);
		uint32_t Length = p_Samples[i]->GetLength() << ((Sample->Get16Bit() ? 1 : 0) + (Sample->GetStereo() ? 1 : 0));
		if ((Sample->Flags & 0x01) == 0)
		{
			p_PCM[i] = nullptr;
			continue;
		}
		p_PCM[i] = new uint8_t[Length];
		if (fd.seek(Sample->SamplePos, SEEK_SET) != Sample->SamplePos)
			throw ModuleLoaderError(E_BAD_IT);
		if ((Sample->Flags & 0x08) != 0)
		{
			if (Sample->Get16Bit())
				itUnpackPCM16(Sample, reinterpret_cast<uint16_t *>(p_PCM[i]), fd, p_Header->FormatVersion > 214 && Sample->Packing & 0x04);
			else
				itUnpackPCM8(Sample, p_PCM[i], fd, p_Header->FormatVersion > 214 && Sample->Packing & 0x04);
		}
		else if (!fd.read(p_PCM[i], Length))
			throw ModuleLoaderError(E_BAD_IT);
		if ((Sample->Packing & 0x01) == 0)
		{
			uint32_t j;
			if (Sample->Get16Bit())
			{
				uint16_t *pcm = (uint16_t *)p_PCM[i];
				for (j = 0; j < (Length >> 1); j++)
					pcm[j] ^= 0x8000;
			}
			else
			{
				uint8_t *pcm = p_PCM[i];
				for (j = 0; j < Length; j++)
					pcm[j] ^= 0x80;
			}
		}
		if (Sample->GetStereo())
		{
			uint8_t *outBuff = new uint8_t[Length];
			if (Sample->Get16Bit())
				stereoInterleave(reinterpret_cast<uint16_t *>(p_PCM[i]), reinterpret_cast<uint16_t *>(outBuff), p_Samples[i]->GetLength());
			else
				stereoInterleave(p_PCM[i], outBuff, p_Samples[i]->GetLength());
			delete [] p_PCM[i];
			p_PCM[i] = outBuff;
		}
	}
}

#undef ModuleLoaderError

ModuleLoaderError::ModuleLoaderError(uint32_t error) : Error(error) { }

const char *ModuleLoaderError::error() const noexcept
{
	switch (Error)
	{
		case E_BAD_MOD:
			return "Bad ProTracker Module";
		case E_BAD_S3M:
			return "Bad Scream Tracker III Module";
		case E_BAD_STM:
			return "Bad Scream Tracker Module - Maybe just song data?";
		case E_BAD_AON:
			return "Bad Art Of Noise Module";
		case E_BAD_FC1x:
			return "Bad Future Composer Module";
		case E_BAD_IT:
			return "Bad Impulse Tracker Module";
		default:
			return "Unknown error";
	}
}

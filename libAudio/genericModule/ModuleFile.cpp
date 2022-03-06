// SPDX-License-Identifier: BSD-3-Clause
#include "genericModule.h"

moduleFile_t::moduleFile_t(audioType_t type, fd_t &&fd) noexcept : audioFile_t(type, std::move(fd)), ctx(makeUnique<decoderContext_t>()) { }

constexpr ModuleFile::ModuleFile(const uint8_t moduleType) noexcept : ModuleType{moduleType}, p_Header{nullptr},
	p_Samples{nullptr}, p_Patterns{nullptr}, p_Instruments{nullptr}, p_PCM{nullptr}, lengthPCM{}, nPCM{},
	MixSampleRate{}, MixBitsPerSample{}, TickCount{}, SamplesToMix{}, MinPeriod{}, MaxPeriod{}, MixChannels{},
	Row{}, NextRow{}, Rows{}, MusicSpeed{}, MusicTempo{}, Pattern{}, NewPattern{}, NextPattern{}, RowsPerBeat{},
	SamplesPerTick{}, Channels{nullptr}, nMixerChannels{}, MixerChannels{nullptr}, globalVolume{},
	globalVolumeSlide{}, PatternDelay{}, FrameDelay{}, MixBuffer{}, DCOffsR{}, DCOffsL{} { }

ModuleFile::ModuleFile(const modMOD_t &file) : ModuleFile{MODULE_MOD}
{
	const fd_t &fd = file.fd();

	p_Header = new ModuleHeader(file);
	if (fd.seek(20, SEEK_SET) != 20)
		throw ModuleLoaderError(E_BAD_MOD);
	p_Samples = new ModuleSample *[p_Header->nSamples];
	for (uint16_t i = 0; i < p_Header->nSamples; i++)
		p_Samples[i] = ModuleSample::LoadSample(file, i);
	if (!fd.seekRel(130 + (p_Header->nSamples != 15 ? 4 : 0)))
		throw ModuleLoaderError(E_BAD_MOD);

	// Count the number of patterns present
	uint32_t maxPattern{};
	for (uint16_t i = 0; i < p_Header->nOrders; i++)
	{
		if (p_Header->Orders[i] < 128)
			maxPattern = std::max<uint32_t>(maxPattern, p_Header->Orders[i]);
	}
	p_Header->nPatterns = maxPattern + 1;
	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	for (uint16_t i = 0; i < p_Header->nPatterns; i++)
		p_Patterns[i] = new ModulePattern(file, p_Header->nChannels);

	modLoadPCM(fd);
	MinPeriod = 56;
	MaxPeriod = 7040;
}

ModuleFile::ModuleFile(const modS3M_t &file) : ModuleFile{MODULE_S3M}
{
	const fd_t &fd = file.fd();

	p_Header = new ModuleHeader(file);
	p_Samples = new ModuleSample *[p_Header->nSamples];
	uint16_t *const SamplePtrs = p_Header->SamplePtrs.get<uint16_t>();
	for (uint16_t i = 0; i < p_Header->nSamples; ++i)
	{
		const uint32_t offset = uint32_t{SamplePtrs[i]} << 4U;
		if (fd.seek(offset, SEEK_SET) != offset)
			throw ModuleLoaderError{E_BAD_S3M};
		p_Samples[i] = ModuleSample::LoadSample(file, i);
	}

	// Count the number of channels present
	p_Header->nChannels = 32;
	for (uint8_t i = 0; i < 32; ++i)
	{
		if (p_Header->ChannelSettings[i] & 0x80)
		{
			p_Header->nChannels = i;
			break;
		}
	}

	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	uint16_t *const PatternPtrs = p_Header->PatternPtrs.get<uint16_t>();
	for (uint16_t i = 0; i < p_Header->nPatterns; ++i)
	{
		const uint32_t offset = uint32_t{PatternPtrs[i]} << 4;
		if (fd.seek(offset, SEEK_SET) != offset)
			throw ModuleLoaderError{E_BAD_S3M};
		p_Patterns[i] = new ModulePattern(file, p_Header->nChannels);
	}

	s3mLoadPCM(fd);
	MinPeriod = 64;
	MaxPeriod = 32767;
}

ModuleFile::ModuleFile(const modSTM_t &file) : ModuleFile{MODULE_STM}
{
	const fd_t &fd = file.fd();

	p_Header = new ModuleHeader(file);
	p_Samples = new ModuleSample *[p_Header->nSamples];
	for (uint16_t i = 0; i < p_Header->nSamples; i++)
		p_Samples[i] = ModuleSample::LoadSample(file, i);
	if (!fd.seekRel(128))
		throw ModuleLoaderError(E_BAD_STM);
	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	for (uint16_t i = 0; i < p_Header->nPatterns; i++)
		p_Patterns[i] = new ModulePattern(file);
	const uint32_t pcmOffset = 1104 + (1024 * p_Header->nPatterns);
	if (fd.seek(pcmOffset, SEEK_SET) != pcmOffset)
		throw ModuleLoaderError(E_BAD_STM);

	stmLoadPCM(fd);
	MinPeriod = 64;
	MaxPeriod = 32767;
}

// http://www.tigernt.com/onlineDoc/68000.pdf
// http://eab.abime.net/showthread.php?t=21516
// ftp://ftp.modland.com/pub/documents/format_documentation/Art%20Of%20Noise%20(.aon).txt
ModuleFile::ModuleFile(const modAON_t &file) : ModuleFile{MODULE_AON}
{
	std::array<char, 4> blockName{};
	uint32_t blockLen = 0;
	uint32_t i, SampleLengths;
	uint8_t ChannelMul;
	const fd_t &fd = file.fd();

	p_Header = new ModuleHeader(file);

	if (!fd.read(blockName) ||
		memcmp(blockName.data(), "PATT", 4) != 0 ||
		!fd.readBE(blockLen))
		throw ModuleLoaderError(E_BAD_AON);
	// 2 if 8 voices, 1 otherwise
	ChannelMul = p_Header->nChannels >> 2;
	// Transform that into a shift value to get the number of patterns
	ChannelMul += 9;
	if ((blockLen % (1 << ChannelMul)) != 0)
		throw ModuleLoaderError(E_BAD_AON);
	p_Header->nPatterns = blockLen >> ChannelMul;
	p_Patterns = new ModulePattern *[p_Header->nPatterns];
	for (i = 0; i < p_Header->nPatterns; i++)
		p_Patterns[i] = new ModulePattern(file, p_Header->nChannels);

	if (!fd.read(blockName) ||
		memcmp(blockName.data(), "INST", 4) != 0 ||
		!fd.readBE(blockLen) ||
		(blockLen % 32) != 0)
		throw ModuleLoaderError(E_BAD_AON);
	p_Header->nSamples = blockLen >> 5;
	const off_t InstrPos = fd.tell();
	if (!fd.seekRel(blockLen) ||
		!fd.read(blockName) ||
		!fd.readBE(blockLen))
		throw ModuleLoaderError(E_BAD_AON);
	// We don't care about the instrument names, so skip over them.
	else if (memcmp(blockName.data(), "INAM", 4) != 0)
	{
		if (!fd.seekRel(blockLen) ||
			!fd.read(blockName) ||
			!fd.readBE(blockLen))
			throw ModuleLoaderError(E_BAD_AON);
	}
	if (memcmp(blockName.data(), "WLEN", 4) != 0 ||
		blockLen != 0x0100)
		throw ModuleLoaderError(E_BAD_AON);

	lengthPCM = makeUnique<uint32_t []>(64);
	for (i = 0, SampleLengths = 0; i < 64; i++)
	{
		if (!fd.readBE(lengthPCM[i]))
			throw ModuleLoaderError(E_BAD_AON);
		SampleLengths += lengthPCM[i];
		if (lengthPCM[i] != 0)
			nPCM = i + 1;
	}
	const off_t PCMPos = fd.tell();

	p_Samples = new ModuleSample *[p_Header->nSamples];
	for (i = 0; i < p_Header->nSamples; i++)
	{
		const off_t offset = InstrPos + (i << 5);
		if (fd.seek(offset, SEEK_SET) != offset)
			throw ModuleLoaderError(E_BAD_AON);
		p_Samples[i] = ModuleSample::LoadSample(file, i, nullptr, lengthPCM.get());
	}

	if (fd.seek(PCMPos, SEEK_SET) != PCMPos ||
		!fd.read(blockName) ||
		memcmp(blockName.data(), "WAVE", 4) != 0 ||
		!fd.readBE(blockLen) ||
		blockLen != SampleLengths)
		throw ModuleLoaderError(E_BAD_AON);

	aonLoadPCM(fd);
	MinPeriod = 56;
	MaxPeriod = 7040;
}

#ifdef ENABLE_FC1x
ModuleFile::ModuleFile(const modFC1x_t &file) : ModuleFile{MODULE_FC1x}
{
//	const fd_t &fd = file.fd();

	p_Header = new ModuleHeader(file);
}
#endif

ModuleFile::ModuleFile(const modIT_t &file) : ModuleFile{MODULE_IT}
{
	const fd_t &fd = file.fd();

	p_Header = new ModuleHeader(file);
	if (p_Header->nInstruments)
	{
		p_Instruments = new ModuleInstrument *[p_Header->nInstruments];
		auto *const instrOffsets = p_Header->InstrumentPtrs.get<uint32_t>();
		for (uint16_t i = 0; i < p_Header->nInstruments; ++i)
		{
			if (fd.seek(instrOffsets[i], SEEK_SET) != instrOffsets[i])
				throw ModuleLoaderError(E_BAD_IT);
			p_Instruments[i] = ModuleInstrument::LoadInstrument(file, i, p_Header->FormatVersion).release();
		}
	}
	p_Samples = new ModuleSample *[p_Header->nSamples];
	auto *const sampleOffsets = p_Header->SamplePtrs.get<uint32_t>();
	for (uint16_t i = 0; i < p_Header->nSamples; ++i)
	{
		if (fd.seek(sampleOffsets[i], SEEK_SET) != sampleOffsets[i])
			throw ModuleLoaderError(E_BAD_IT);
		p_Samples[i] = ModuleSample::LoadSample(file, i);
	}

	p_Header->nChannels = 64;
	for (uint8_t i = 0; i < 64; i++)
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
	for (uint16_t i = 0; i < p_Header->nPatterns; i++)
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

	itLoadPCM(fd);
	MinPeriod = 8;
	MaxPeriod = 61440;//32767;
}

ModuleFile::~ModuleFile()
{
	uint32_t i;

	DeinitMixer();

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

void ModuleFile::modLoadPCM(const fd_t &fd)
{
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

void ModuleFile::s3mLoadPCM(const fd_t &fd)
{
	p_PCM = new uint8_t *[p_Header->nSamples];
	for (uint32_t i = 0; i < p_Header->nSamples; ++i)
	{
		const uint32_t length = p_Samples[i]->GetLength() << (p_Samples[i]->Get16Bit() ? 1 : 0);
		if (length != 0 && p_Samples[i]->GetType() == 1)
		{
			const auto *sample = dynamic_cast<ModuleSampleNative *>(p_Samples[i]);
			const uint32_t offset = uint32_t{sample->SamplePos} << 4U;
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
						pcm[j] ^= 0x8000U;
				}
				else
				{
					auto *pcm = reinterpret_cast<uint8_t *>(p_PCM[i]);
					for (uint32_t j = 0; j < length; j++)
						pcm[j] ^= 0x80U;
				}
			}
		}
		else
			p_PCM[i] = nullptr;
	}
}

void ModuleFile::stmLoadPCM(const fd_t &fd)
{
	p_PCM = new uint8_t *[p_Header->nSamples];
	for (uint16_t i = 0; i < p_Header->nSamples; i++)
	{
		const uint32_t length = p_Samples[i]->GetLength();
		if (length != 0)
		{
			p_PCM[i] = new uint8_t[length];
			if (!fd.read(p_PCM[i], length) ||
				!fd.seekRel(length % 16))
				throw ModuleLoaderError(E_BAD_STM);
		}
		else
			p_PCM[i] = nullptr;
	}
}

void ModuleFile::aonLoadPCM(const fd_t &fd)
{
	p_PCM = new uint8_t *[nPCM];
	for (uint32_t i = 0; i < nPCM; i++)
	{
		uint32_t Length = lengthPCM[i];
		if (Length != 0)
		{
			p_PCM[i] = new uint8_t[Length];
			if (fd.read(p_PCM[i], Length))
				throw ModuleLoaderError(E_BAD_AON);
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
			uint16_t bits = uint16_t(itBitstreamRead(buff, buffLen, fd, bitWidth));
			if (fd.isEOF())
				return;
			if (bitWidth < 7)
			{
				uint16_t special = 1 << (bitWidth - 1);
				if (bits == special)
				{
					const uint8_t bits = itBitstreamRead(buff, buffLen, fd, 3) + 1;
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
						bitWidth = uint8_t(bits);
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
				uint32_t special = 1U << (bitWidth - 1U);
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

void ModuleFile::itLoadPCM(const fd_t &fd)
{
	p_PCM = new uint8_t *[p_Header->nSamples];
	for (uint32_t i = 0; i < p_Header->nSamples; ++i)
	{
		const auto Sample = static_cast<ModuleSampleNative *>(p_Samples[i]);
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

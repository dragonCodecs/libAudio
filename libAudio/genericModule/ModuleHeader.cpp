// SPDX-License-Identifier: BSD-3-Clause
#include "genericModule.h"

using substrate::make_unique;
using substrate::make_managed;

// Default initalise key fields
ModuleHeader::ModuleHeader() noexcept { Volumes.fill(64); }

ModuleHeader::ModuleHeader(const modMOD_t &file) : ModuleHeader{}
{
	constexpr const uint32_t seekOffset = (30 * 31) + 130;
	std::array<char, 4> magic{};
	const fd_t &fd = file.fd();

	Name = make_unique<char []>(21);
	Orders = make_unique<uint8_t []>(128);

	if (!Name || !Orders ||
		!fd.read(Name, 20) ||
		fd.seek(seekOffset, SEEK_CUR) != (seekOffset + 20) ||
		!fd.read(magic) ||
		fd.seek(-134, SEEK_CUR) != (seekOffset - 130 + 20) ||
		!fd.read(&nOrders, 1) ||
		!fd.read(&RestartPos, 1) ||
		!fd.read(Orders.get(), 128))
		throw ModuleLoaderError{E_BAD_MOD};

	// Defaults
	nSamples = 31;
	nChannels = 4;
	// Now find out the actual values
	if (strncmp(magic.data(), "M.K.", 4) == 0 || strncmp(magic.data(), "M!K!", 4) == 0 ||
		strncmp(magic.data(), "M&K!", 4) == 0 || strncmp(magic.data(), "N.T.", 4) == 0)
		nChannels = 4;
	else if (strncmp(magic.data(), "CD81", 4) == 0 || strncmp(magic.data(), "OKTA", 4) == 0)
		nChannels = 8;
	else if (strncmp(magic.data(), "FLT", 3) == 0 && magic[3] >= '4' && magic[3] <= '9')
		nChannels = magic[3] - '0';
	else if (strncmp(magic.data() + 1, "CHN", 3) == 0 && magic[0] >= '4' && magic[0] <= '9')
		nChannels = magic[0] - '0';
	else if (strncmp(magic.data() + 2, "CH", 2) == 0 && magic[0] == '1' && magic[1] >= '0' && magic[1] <= '9')
		nChannels = magic[1] - '0' + 10;
	else if (strncmp(magic.data() + 2, "CH", 2) == 0 && magic[0] == '2' && magic[1] >= '0' && magic[1] <= '9')
		nChannels = magic[1] - '0' + 20;
	else if (strncmp(magic.data() + 2, "CH", 2) == 0 && magic[0] == '3' && magic[1] >= '0' && magic[1] <= '9')
		nChannels = magic[1] - '0' + 30;
	else if (strncmp(magic.data(), "TDZ", 3) == 0 && magic[3] >= '4' && magic[3] <= '9')
		nChannels = magic[3] - '0';
	else if (strncmp(magic.data(), "16CN", 4) == 0)
		nChannels = 16;
	else if (strncmp(magic.data(), "32CN", 4) == 0)
		nChannels = 32;
	else
		nSamples = 15;

	if (Name[19] != 0)
		Name[20] = 0;
	if (nOrders > 128)
		nOrders = 128;
	if (RestartPos > 127)
		RestartPos = 127;
}

ModuleHeader::ModuleHeader(const modS3M_t &file) : ModuleHeader{}
{
	std::array<char, 4> magic{};
	std::array<uint8_t, 10> dontCare{};
	uint8_t Const{};
	uint16_t Special{};
	uint16_t rawFlags{};
	const fd_t &fd = file.fd();

	Name = make_unique<char []>(29);
	if (!Name ||
		!fd.read(Name, 28) ||
		!fd.read(Const) ||
		!fd.read(Type) ||
		!fd.read<2>(dontCare) ||
		!fd.read(nOrders) ||
		!fd.read(nSamples) ||
		!fd.read(nPatterns) ||
		!fd.read(rawFlags) ||
		!fd.read(CreationVersion) ||
		!fd.read(FormatVersion) ||
		!fd.read(magic) ||
		!fd.read(GlobalVolume) ||
		!fd.read(InitialSpeed) ||
		!fd.read(InitialTempo) ||
		!fd.read(MasterVolume) ||
		!fd.read(dontCare) ||
		!fd.read(Special) ||
		!fd.read(ChannelSettings, 32))
		throw ModuleLoaderError{E_BAD_S3M};

	if (Name[27] != 0)
		Name[28] = 0;
	if ((rawFlags & 0x04U) != 0)
		Flags |= FILE_FLAGS_AMIGA_SLIDES;
	if ((rawFlags & 0x10U) != 0)
		Flags |= FILE_FLAGS_AMIGA_LIMITS;
	if (CreationVersion < 0x1320U && (rawFlags & 0x40U) != 0)
		Flags |= FILE_FLAGS_FAST_SLIDES;

	if (Const != 0x1AU || Type != 16 || FormatVersion > 2 || FormatVersion == 0 ||
		memcmp(magic.data(), "SCRM", 4) != 0)
		throw ModuleLoaderError{E_BAD_S3M};

	Orders = make_unique<uint8_t []>(nOrders);
	SamplePtrs = make_managed<uint16_t []>(nSamples);
	PatternPtrs = make_managed<uint16_t []>(nPatterns);
	if (!Orders || !SamplePtrs || !PatternPtrs ||
		!fd.read(Orders.get(), nOrders) ||
		!fd.read(SamplePtrs.get(), static_cast<size_t>(nSamples) * 2U) ||
		!fd.read(PatternPtrs.get(), static_cast<size_t>(nPatterns) * 2U))
		throw ModuleLoaderError{E_BAD_S3M};

	// Panning?
	if (dontCare[1] == 0xFCU)
	{
		Panning = make_unique<uint16_t []>(32);
		if (!Panning)
			throw ModuleLoaderError{E_BAD_S3M};
		for (size_t i = 0; i < 32U; ++i)
		{
			uint8_t value{};
			if (!fd.read(value))
				throw ModuleLoaderError{E_BAD_S3M};
			else if (value & 0x20U)
				Panning[i] = ((value & 0x0FU) << 4U) | (value & 0x0FU);
			else
				Panning[i] = 128;
		}
	}
}

ModuleHeader::ModuleHeader(const modSTM_t &file) : ModuleHeader{}
{
	std::array<char, 9> magic{};
	std::array<char, 13> reserved{};
	uint8_t patternCount_{};
	const fd_t &fd = file.fd();

	nOrders = 128;
	Name = make_unique<char []>(21);
	Orders = make_unique<uint8_t []>(nOrders);
	if (!Name || !Orders ||
		!fd.read(Name, 20) ||
		!fd.read(magic) ||
		strncmp(magic.data(), "!Scream!\x1A", 9) != 0 ||
		!fd.read(Type) || Type != 2 ||
		!fd.read(FormatVersion) ||
		!fd.read(InitialSpeed) ||
		!fd.read(patternCount_) ||
		!fd.read(GlobalVolume) ||
		!fd.read(reserved) ||
		fd.seek(1040, SEEK_SET) != 1040 ||
		!fd.read(Orders, nOrders) ||
		fd.seek(48, SEEK_SET) != 48)
		throw ModuleLoaderError{E_BAD_STM};

	InitialSpeed >>= 4U;
	nPatterns = patternCount_;
	if (Name[19] != 0)
		Name[20] = 0;

	for (uint16_t i{}; i < nOrders; ++i)
	{
		if (Orders[i] >= 99)
			Orders[i] = 255;
	}
	nSamples = 31;
	nChannels = 4;
}

ModuleHeader::ModuleHeader(const modAON_t &file) : ModuleHeader{}
{
	std::array<char, 4> magic1{};
	std::array<char, 4> blockName{};
	std::array<char, 42> magic2{};
	uint32_t blockLen = 0;
	uint8_t Const{};
	const fd_t &fd = file.fd();

	if (!fd.read(magic1) ||
		!fd.read(magic2) ||
		memcmp(magic1.data(), "AON", 3) != 0 ||
		(magic1[3] != '4' && magic1[3] != '8') ||
		memcmp(magic2.data(), "artofnoise by bastian spiegel (twice/lego)", 42) != 0 ||
		!fd.read(blockName) ||
		memcmp(blockName.data(), "NAME", 4) != 0 ||
		!fd.readBE(blockLen))
		throw ModuleLoaderError{E_BAD_AON};
	nChannels = magic1[3] - '0';
	Name = make_unique<char []>(blockLen + 1);
	if (!Name ||
		!fd.read(Name, blockLen))
		throw ModuleLoaderError{E_BAD_AON};
	Name[blockLen] = 0;
	if (!fd.read(blockName) ||
		memcmp(blockName.data(), "AUTH", 4) != 0 ||
		!fd.readBE(blockLen))
		throw ModuleLoaderError{E_BAD_AON};
	Author = make_unique<char []>(blockLen + 1);
	if (!Author ||
		!fd.read(Author, blockLen))
		throw ModuleLoaderError{E_BAD_AON};
	Author[blockLen] = 0;
	if (!fd.read(blockName) ||
		memcmp(blockName.data(), "DATE", 4) != 0 ||
		!fd.readBE(blockLen) ||
		!fd.seekRel(blockLen) ||
		!fd.read(blockName) ||
		memcmp(blockName.data(), "RMRK", 4) != 0 ||
		!fd.readBE(blockLen))
		throw ModuleLoaderError{E_BAD_AON};
	else if (blockLen)
	{
		Remark = make_unique<char []>(blockLen + 1);
		if (!fd.read(Remark, blockLen))
			throw ModuleLoaderError{E_BAD_AON};
		Remark[blockLen] = 0;
	}
	else
		Remark = nullptr;
	if (!fd.read(blockName) ||
		memcmp(blockName.data(), "INFO", 4) != 0 ||
		!fd.readBE(blockLen) ||
		!fd.read(Const) ||
		blockLen != 4 ||
		Const != 0x34 ||
		!fd.read(Const))
		throw ModuleLoaderError{E_BAD_AON};
	nOrders = Const;
	if (!fd.read(Const))
		throw ModuleLoaderError{E_BAD_AON};
	RestartPos = Const;
	if (!fd.read(Const) ||
		// Skip over the arpeggio table..
		!fd.read(blockName) ||
		memcmp(blockName.data(), "ARPG", 4) != 0 ||
		!fd.readBE(blockLen) ||
		blockLen != 64)
		throw ModuleLoaderError{E_BAD_AON};
	for (size_t i{}; i < 16; ++i)
	{
		for (size_t j{}; j < 4; ++j)
		{
			if (!fd.read(ArpTable[i][j]))
				throw ModuleLoaderError{E_BAD_AON};
		}
	}
	if (ArpTable[0][0] != 0 || ArpTable[0][1] != 0 ||
		ArpTable[0][2] != 0 || ArpTable[0][3] != 0)
		throw ModuleLoaderError{E_BAD_AON};
	else if (!fd.read(blockName) ||
		memcmp(blockName.data(), "PLST", 4) != 0 ||
		!fd.readBE(blockLen))
		throw ModuleLoaderError{E_BAD_AON};
	// If odd number of orders
	if ((nOrders & 1U) != 0)
		// Get rid of the fill byte from the count
		blockLen--;
	Orders = make_unique<uint8_t []>(nOrders);
	if (blockLen != nOrders || !Orders)
		throw ModuleLoaderError{E_BAD_AON};
	for (uint16_t i = 0; i < nOrders; i++)
	{
		if (!fd.read(Orders[i]))
			throw ModuleLoaderError{E_BAD_AON};
	}
	// If odd read length, read the fill byte
	if ((blockLen & 1U) != 0 &&
		!fd.read(Const))
		throw ModuleLoaderError{E_BAD_AON};
}

#ifdef ENABLE_FC1x
ModuleHeader::ModuleHeader(const modFC1x_t &file) : ModuleHeader{}
{
	std::array<char, 4> fc1xMagic;
	const fd_t &fd = file.fd();

	if (!fd.read(fc1xMagic) ||
		(memcmp(fc1xMagic.data(), "SMOD", 4) != 0 &&
		memcmp(fc1xMagic.data(), "FC14", 4) != 0) ||
		!fd.read(SeqLength) ||
		!fd.read(PatternOffs) ||
		!fd.read(PatLength) ||
		!fd.read(FrequenciesOffs) ||
		!fd.read(FrequenciesLength) ||
		!fd.read(VolumeOffs) ||
		!fd.read(VolumeLength) ||
		!fd.read(SampleOffs) ||
		!fd.read(SampleLength))
		throw ModuleLoaderError{E_BAD_FC1x};
}
#endif

ModuleHeader::ModuleHeader(const modIT_t &file) : ModuleHeader{}
{
	std::array<char, 4> magic{};
	std::array<char, 4> dontCare{};
	uint16_t msgLength{};
	uint16_t songFlags{};
	uint8_t Const{};
	const fd_t &fd = file.fd();

	if (!fd.read(magic) ||
		strncmp(magic.data(), "IMPM", 4) != 0)
		throw ModuleLoaderError{E_BAD_IT};

	Name = make_unique<char []>(27);
	Panning = make_unique<uint16_t []>(64);

	if (!Name || !Panning ||
		!fd.read(Name, 26) ||
		!fd.read<2>(dontCare) ||
		!fd.readLE(nOrders) ||
		!fd.readLE(nInstruments) ||
		!fd.readLE(nSamples) ||
		!fd.readLE(nPatterns) ||
		!fd.readLE(CreationVersion) ||
		!fd.readLE(FormatVersion) ||
		!fd.readLE(songFlags) ||
		// TODO: Handle special.
		!fd.read<2>(dontCare) ||
		!fd.read(GlobalVolume) ||
		!fd.read(MasterVolume) ||
		!fd.read(InitialSpeed) ||
		!fd.read(InitialTempo) ||
		!fd.read(Separation) ||
		!fd.read(Const) ||
		!fd.read(msgLength) ||
		!fd.readLE(MessageOffs) ||
		!fd.read(dontCare))
		throw ModuleLoaderError{E_BAD_IT};

	if (Name[25] != 0)
		Name[26] = 0;

	// This loop is unfortunately necessary to perform a width conversion
	// from the read value to our internal panning value.
	for (size_t i = 0; i < 64; i++)
	{
		uint8_t value{};
		if (!fd.read(value))
			throw ModuleLoaderError{E_BAD_IT};
		Panning[i] = value;
	}

	Orders = make_unique<uint8_t []>(nOrders);
	InstrumentPtrs = make_managed<uint32_t []>(nInstruments);
	// TODO: Implement managedPtr_t<> to handle type elision of uint32_t to void
	// for storage and retrieval + proper deletion of these.
	SamplePtrs = make_managed<uint32_t []>(nSamples);
	PatternPtrs = make_managed<uint32_t []>(nPatterns);

	if (!Orders || !InstrumentPtrs || !SamplePtrs || !PatternPtrs ||
		!fd.read(Volumes) ||
		!fd.read(Orders, nOrders) ||
		!fd.read(InstrumentPtrs, static_cast<size_t>(nInstruments) * 4U) ||
		!fd.read(SamplePtrs, static_cast<size_t>(nSamples) * 4U) ||
		!fd.read(PatternPtrs, static_cast<size_t>(nPatterns) * 4U))
		throw ModuleLoaderError{E_BAD_IT};

	Flags = 0;
	if (songFlags & 0x0008U)
		Flags |= FILE_FLAGS_LINEAR_SLIDES;
	if (songFlags & 0x0010U)
		Flags |= FILE_FLAGS_OLD_IT_EFFECTS;
	else
		Flags |= FILE_FLAGS_AMIGA_SLIDES;
	if (!(songFlags & 0x0004U))
		nInstruments = 0;

	if (MessageOffs != 0)
	{
		Remark = make_unique<char []>(msgLength + 1);
		if (fd.seek(MessageOffs, SEEK_SET) != MessageOffs ||
			!fd.read(Remark, msgLength))
			throw ModuleLoaderError{E_BAD_IT};
		Remark[msgLength] = 0;
	}
}

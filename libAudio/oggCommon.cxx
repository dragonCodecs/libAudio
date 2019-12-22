#include <array>
#include <string.h>

#include "fd.hxx"
#include "uniquePtr.hxx"
#include "oggCommon.hxx"

inline bool clonePacketData(ogg_packet &packet) noexcept try
{
	unsigned char *data = packet.packet;
	packet.packet = makeUniqueT<unsigned char []>(packet.bytes).release();
	memcpy(packet.packet, data, packet.bytes);
	return packet.packet;
}
catch (std::bad_alloc &)
	{ return false; }

inline void freePacketData(ogg_packet &packet) noexcept
	{ delete [] packet.packet; }

inline bool oggCleanup(ogg_sync_state &syncState, ogg_stream_state &streamState) noexcept
{
	ogg_stream_clear(&streamState);
	syncState.data = nullptr;
	ogg_sync_clear(&syncState);
	return false;
}

bool isOgg(const int32_t fd, ogg_packet &headerPacket) noexcept
{
	std::array<unsigned char, 79> header{};
	if (fd == -1 ||
		read(fd, header.data(), header.size()) != header.size() ||
		lseek(fd, 0, SEEK_SET) != 0 ||
		memcmp(header.data(), "OggS", 4) != 0)
		return false;
	// The following rash of call puke pulls apart the first Ogg page we
	// just read from the file and populates headerPacket with the resulting
	// decoder-specific data. This is the only way to get a clean check for
	// each of the supported codec encapsulations.
	ogg_sync_state syncState{};
	ogg_stream_state streamState{};
	ogg_sync_init(&syncState);
	ogg_stream_init(&streamState, -1);
	syncState.data = header.data();
	syncState.storage = header.size();
	if (ogg_sync_wrote(&syncState, header.size()))
		return oggCleanup(syncState, streamState);
	ogg_page page{};
	if (ogg_sync_pageout(&syncState, &page) != 1 ||
		ogg_stream_reset_serialno(&streamState, ogg_page_serialno(&page)) ||
		ogg_stream_pagein(&streamState, &page) ||
		ogg_stream_packetout(&streamState, &headerPacket) != 1)
		return oggCleanup(syncState, streamState);
	clonePacketData(headerPacket);
	oggCleanup(syncState, streamState);
	return true;
}

bool isVorbis(ogg_packet &headerPacket) noexcept
{
	const bool result = headerPacket.bytes < 7 ||
		headerPacket.packet[0] != 1 ||
		memcmp(headerPacket.packet + 1, "vorbis", 6) != 0;
	freePacketData(headerPacket);
	return !result;
}

bool isFLAC(ogg_packet &headerPacket) noexcept
{
	const bool result = headerPacket.bytes < 13 ||
		headerPacket.packet[0] != 0x7F ||
		memcmp(headerPacket.packet + 1, "FLAC", 4) != 0 ||
		memcmp(headerPacket.packet + 9, "fLaC", 4) != 0;
	freePacketData(headerPacket);
	return !result;
}

bool isOpus(ogg_packet &headerPacket) noexcept
{
	const bool result = headerPacket.bytes < 9 ||
		memcmp(headerPacket.packet, "OpusHead", 8) != 0;
	freePacketData(headerPacket);
	return !result;
}

// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2009-2023 Rachel Mant <git@dragonmux.network>
#include <guiddef.h>

#ifndef QWORD
typedef unsigned __int64 QWORD;
#endif

#include "WMA_HuffTables.h"

enum data_types : WORD
{
	UnicodeStr = 0,
	ANSIStr = 1,
	Boolean = 2,
	DoubleWord = 3,
	QuadWord = 4,
	Word = 5
};

#pragma pack(push, 1)

typedef struct _ASFBlock
{
	GUID guid;
	QWORD size;
} ASFBlock;

typedef struct _ASFFileHeader
{
	GUID guid;
	QWORD size;
	DWORD header_objects;
	BYTE reserved[2];
} ASFFileHeader;

typedef struct _ASFMainHeader
{
	GUID file_guid;
	QWORD file_size;
	QWORD create_time;
	QWORD data_packets;
	QWORD play_time;
	QWORD send_time;
	QWORD preroll;           /**< timestamp of the first packet, in milliseconds
                                 *   if nonzero - substract from time */
	DWORD flags;
	DWORD min_pktsize;
	DWORD max_pktsize;
	DWORD max_bitrate;
} ASFMainHeader;

typedef struct _ASFStreamHeader
{
	GUID strm_guid;
	GUID errcorr_guid;
	QWORD time_offset;
	DWORD data_len;
	DWORD errcorr_len;
	WORD flags;
	DWORD reserved;
	BYTE *data;
	BYTE *errcorr;
} ASFStreamHeader;

typedef struct _ASFExtHeader
{
	GUID reserved1;
	WORD reserved2;
	DWORD data_len;
	BYTE *data;
} ASFExtHeader;

typedef struct _ASFCodecEntries
{
	WORD type;
	WORD name_len;
	WCHAR *name;
	WORD description_len;
	WCHAR *description;
	WORD information_len;
	BYTE *information;
} ASFCodecEntries;

typedef struct _ASFCodecList
{
	GUID reserved;
	DWORD codec_count;
	std::vector<ASFCodecEntries *> entries;
} ASFCodecList;

typedef struct _ASFExtContentDesc
{
	WORD name_len;
	WCHAR *name;
	WORD value_type;
	WORD value_len;
	BYTE *value;
} ASFExtContentDesc;

typedef struct _ASFExtContent
{
	WORD descriptor_count;
	std::vector<ASFExtContentDesc *> descriptors;
} ASFExtContent;

typedef struct _ASFBitrateRecord
{
	WORD flags;
	DWORD average;
} ASFBitrateRecord;

typedef struct _ASFStreamBitrate
{
	WORD record_count;
	std::vector<ASFBitrateRecord *> records;
} ASFStreamBitrate;

typedef struct _ASFContentDesc
{
	WORD title_len;
	WORD author_len;
	WORD cright_len;
	WORD desc_len;
	WORD rating_len;
	WCHAR *title;
	WCHAR *author;
	WCHAR *cright;
	WCHAR *desc;
	WCHAR *rating;
} ASFContentDesc;

typedef struct _ASFDataHeader
{
	GUID guid;
	QWORD size;
	GUID file_guid;
	QWORD data_packets;
	WORD reserved;
} ASFDataHeader;

typedef struct _ASFPayloadParseInfo
{
	BYTE length_type_flags;
	BYTE prop_flags;
	DWORD packet_length;
	DWORD sequence;
	DWORD padding_length;
	DWORD send_time;
	WORD duration;
} ASFPayloadParseInfo;

typedef struct _ASFSingleUnComprPayload
{
	BYTE *rep_data;
	BYTE *payload;
} ASFSingleUnComprPayload;

typedef struct _ASFSingleComprPayload
{
	BYTE time_delta;
	BYTE *sub_payload;
} ASFSingleComprPayload;

typedef struct _ASFSinglePayload
{
	BYTE stream_no;
	DWORD media_object_no;
	DWORD media_object_offset;
	DWORD rep_data_len;
	ASFSingleUnComprPayload *uncompressed;
	ASFSingleComprPayload *compressed;
} ASFSinglePayload;

typedef struct _ASFMultiUnComprPayload
{
	BYTE stream_no;
	DWORD media_object_no;
	DWORD media_object_offset;
	DWORD rep_data_len;
	BYTE *rep_data;
	DWORD payload_len;
	BYTE *payload;
} ASFMultiUnComprPayload;

typedef struct _ASFMultiPayload
{
	BYTE payload_flags;
	ASFMultiUnComprPayload *uncompressed;
} ASFMultiPayload;

typedef struct _ASFDataPacket
{
	BYTE flags;
	BYTE *errcorr;
	ASFPayloadParseInfo *parse_info;
	ASFSinglePayload *single_payload;
	ASFMultiPayload *multi_payload;
} ASFDataPacket;

#pragma pack(pop)

static const GUID ASF_HEADER_GUID = {0x75B22630, 0x668E, 0x11CF, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
static const GUID ASF_FILEHEADER_GUID = {0x8CABDCA1, 0xA947, 0x11CF, 0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};
static const GUID ASF_STREAMHEADER_GUID = {0xB7DC0791, 0xA9B7, 0x11CF, 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};
static const GUID ASF_EXTHEADER_GUID = {0x5FBF03B5, 0xA92E, 0x11CF, 0x8E, 0xE3, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};
static const GUID ASF_EXTHEADER2_GUID = {0xABD3D211, 0xA9BA, 0x11CF, 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};
static const GUID ASF_CODECLIST_GUID = {0x86D15240, 0x311D, 0x11D0, 0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6};
static const GUID ASF_CODECLIST2_GUID = {0x86D15241, 0x311D, 0x11D0, 0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6};
static const GUID ASF_EXTCONTENT_GUID = {0xD2D0A440, 0xE307, 0x11D2, 0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50};
static const GUID ASF_STREAMBITRATE_GUID = {0x7BF875CE, 0x468D, 0x11D1, 0x8D, 0x82, 0x00, 0x60, 0x97, 0xC9, 0xA2, 0xB2};
static const GUID ASF_CONTENTDESC_GUID = {0x75B22633, 0x668E, 0x11CF, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
static const GUID ASF_DATAHEADER_GUID = {0x75B22636, 0x668E, 0x11CF, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
static const GUID ASF_AUDIOSTREAM_GUID = {0xF8699E40, 0x5B4D, 0x11CF, 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B};

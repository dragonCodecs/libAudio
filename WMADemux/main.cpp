#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <string>
#include <windows.h>

/*#define _USE_MATH_DEFINES
#include <math.h>*/

#include <guiddef.h>

#ifndef QWORD
typedef unsigned __int64 QWORD;
#endif

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

typedef struct FileInfo
{
	double TotalTime;
	long BitsPerSample;
	long BitRate;
	int Channels;
	//int BitStream;
	char *Title;
	char *Artist;
	char *Album;
	std::vector<char *> OtherComments;
	int nOtherComments;
} FileInfo;

typedef struct _WMA_Intern
{
	FILE *f_WMA;
	FileInfo *p_FI;
	ASFFileHeader *p_InitBlock;
	ASFMainHeader *p_ASFMain;
	std::vector<ASFStreamHeader *> p_ASFStrm;
	ASFExtHeader *p_ASFExt;
	ASFCodecList *p_ASFCodecs;
	ASFExtContent *p_ASFExtContentDesc;
	ASFStreamBitrate *p_ASFStrmBitRate;
	ASFContentDesc *p_ASFContentDesc;
	ASFDataHeader *p_DataBlock;
#ifdef _DEBUG
	int PacketCount;
#endif
} WMA_Intern;

void *wmaOpenR(char *FileName)
{
	WMA_Intern *ret = NULL;
	FILE *f_WMA = NULL;

	ret = (WMA_Intern *)malloc(sizeof(WMA_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(WMA_Intern));

	f_WMA = fopen(FileName, "rb");
	if (f_WMA == NULL)
		return f_WMA;

	ret->f_WMA = f_WMA;
	ret->p_InitBlock = (ASFFileHeader *)malloc(sizeof(ASFFileHeader));

	fread(ret->p_InitBlock, sizeof(ASFFileHeader), 1, f_WMA);

	if (ret->p_InitBlock->guid != ASF_HEADER_GUID)
		return NULL;
	if (ret->p_InitBlock->reserved[0] != 0x01 || ret->p_InitBlock->reserved[1] != 0x02)
		return NULL;

	for (UINT i = 0; i < ret->p_InitBlock->header_objects; i++)
	{
		GUID g;
		QWORD size;
		ASFBlock *block;

		fread(&g, sizeof(GUID), 1, f_WMA);
		fread(&size, sizeof(QWORD), 1, f_WMA);

		block = (ASFBlock *)malloc(sizeof(ASFBlock));
		block->guid = g;
		block->size = size;

		if (g == ASF_FILEHEADER_GUID)
		{
			ret->p_ASFMain = (ASFMainHeader *)malloc(sizeof(ASFMainHeader));
			fread(ret->p_ASFMain, sizeof(ASFMainHeader), 1, f_WMA);
		}
		else if (g == ASF_STREAMHEADER_GUID)
		{
			ASFStreamHeader *stream;
			int dataloc = sizeof(ASFStreamHeader) - (2 * sizeof(BYTE *));

			stream = (ASFStreamHeader *)malloc(sizeof(ASFStreamHeader));
			ret->p_ASFStrm.push_back(stream);
			fread(stream, dataloc, 1, f_WMA);

			stream->data = (BYTE *)malloc(stream->data_len);
			fread(stream->data, stream->data_len, 1, f_WMA);

			stream->errcorr = (BYTE *)malloc(stream->errcorr_len);
			fread(stream->errcorr, stream->errcorr_len, 1, f_WMA);
		}
		else if (g == ASF_EXTHEADER_GUID)
		{
			ASFExtHeader *ext;
			int dataloc = sizeof(ASFExtHeader) - sizeof(BYTE *);

			ext = ret->p_ASFExt = (ASFExtHeader *)malloc(sizeof(ASFExtHeader));
			fread(ext, dataloc, 1, f_WMA);

			ext->data = (BYTE *)malloc(ext->data_len);
			fread(ext->data, ext->data_len, 1, f_WMA);

			if (ext->reserved1 != ASF_EXTHEADER2_GUID ||
				ext->reserved2 != 0x06)
				return NULL;
		}
		else if (g == ASF_CODECLIST_GUID)
		{
			ASFCodecList *codecs = ret->p_ASFCodecs = (ASFCodecList *)malloc(sizeof(ASFCodecList));
			memset(codecs, 0x00, sizeof(ASFCodecList));

			fread(codecs, sizeof(GUID) + sizeof(DWORD), 1, f_WMA);
			if (codecs->reserved != ASF_CODECLIST2_GUID)
				return NULL;

			for (DWORD j = 0; j < codecs->codec_count; j++)
			{
				ASFCodecEntries *entry = (ASFCodecEntries *)malloc(sizeof(ASFCodecEntries));
				codecs->entries.push_back(entry);

				fread(entry, sizeof(WORD), 2, f_WMA);
				entry->name = (WCHAR *)malloc(sizeof(WCHAR) * entry->name_len);
				fread(entry->name, sizeof(WCHAR), entry->name_len, f_WMA);

				fread(&entry->description_len, sizeof(WORD), 1, f_WMA);
				entry->description = (WCHAR *)malloc(sizeof(WCHAR) * entry->description_len);
				fread(entry->description, sizeof(WCHAR), entry->description_len, f_WMA);

				fread(&entry->information_len, sizeof(WORD), 1, f_WMA);
				entry->information = (BYTE *)malloc(entry->information_len);
				fread(entry->information, entry->information_len, 1, f_WMA);
			}
		}
		else if (g == ASF_EXTCONTENT_GUID)
		{
			ASFExtContent *extcont = ret->p_ASFExtContentDesc = (ASFExtContent *)malloc(sizeof(ASFExtContent));
			memset(extcont, 0x00, sizeof(ASFExtContent));

			fread(extcont, sizeof(WORD), 1, f_WMA);

			for (int j = 0; j < extcont->descriptor_count; j++)
			{
				ASFExtContentDesc *desc = (ASFExtContentDesc *)malloc(sizeof(ASFExtContentDesc));
				extcont->descriptors.push_back(desc);
				memset(desc, 0x00, sizeof(ASFExtContentDesc));

				fread(desc, sizeof(WORD), 1, f_WMA);
				desc->name = (WCHAR *)malloc(sizeof(WCHAR) * desc->name_len);
				fread(desc->name, desc->name_len, 1, f_WMA);

				fread(&desc->value_type, sizeof(WORD), 2, f_WMA);
				if (desc->value_type == UnicodeStr || desc->value_type == ANSIStr)
				{
					desc->value = (BYTE *)malloc(desc->value_len);
					fread(desc->value, desc->value_len, 1, f_WMA);
				}
				else if (desc->value_type >= Boolean && desc->value_type <= Word)
				{
					desc->value = (BYTE *)malloc(desc->value_len);
					fread(desc->value, desc->value_len, 1, f_WMA);
				}
			}
		}
		else if (g == ASF_STREAMBITRATE_GUID)
		{
			ASFStreamBitrate *bitrate = ret->p_ASFStrmBitRate = (ASFStreamBitrate *)malloc(sizeof(ASFStreamBitrate));
			memset(bitrate, 0x00, sizeof(ASFStreamBitrate));

			fread(bitrate, sizeof(WORD), 1, f_WMA);

			for (int j = 0; j < bitrate->record_count; j++)
			{
				ASFBitrateRecord *rate = (ASFBitrateRecord *)malloc(sizeof(ASFBitrateRecord));
				bitrate->records.push_back(rate);
				memset(rate, 0x00, sizeof(ASFBitrateRecord));

				fread(rate, sizeof(ASFBitrateRecord), 1, f_WMA);
			}
		}
		else if (g == ASF_CONTENTDESC_GUID)
		{
			ASFContentDesc *desc = ret->p_ASFContentDesc = (ASFContentDesc *)malloc(sizeof(ASFContentDesc));
			memset(desc, 0x00, sizeof(ASFContentDesc));

			fread(desc, sizeof(WORD), 5, f_WMA);
			desc->title = (WCHAR *)malloc(desc->title_len);
			fread(desc->title, desc->title_len, 1, f_WMA);
			desc->author = (WCHAR *)malloc(desc->author_len);
			fread(desc->author, desc->author_len, 1, f_WMA);
			desc->cright = (WCHAR *)malloc(desc->cright_len);
			fread(desc->cright, desc->cright_len, 1, f_WMA);
			desc->desc = (WCHAR *)malloc(desc->desc_len);
			fread(desc->desc, desc->desc_len, 1, f_WMA);
			desc->rating = (WCHAR *)malloc(desc->rating_len);
			fread(desc->rating, desc->rating_len, 1, f_WMA);
		}
		else
			fseek(f_WMA, (size_t)size - 24, SEEK_CUR);
	}

	ret->p_DataBlock = (ASFDataHeader *)malloc(sizeof(ASFDataHeader));
	memset(ret->p_DataBlock, 0x00, sizeof(ASFDataHeader));

	fread(ret->p_DataBlock, sizeof(ASFDataHeader), 1, f_WMA);

	if (ret->p_DataBlock->guid != ASF_DATAHEADER_GUID ||
		ret->p_DataBlock->reserved != 0x0101 ||
		ret->p_DataBlock->file_guid != ret->p_ASFMain->file_guid)
		return NULL;

	return ret;
}

ASFDataPacket *ReadPacket(WMA_Intern *p_WF)
{
	FILE *f_WMA = p_WF->f_WMA;
	ASFDataPacket *ret = (ASFDataPacket *)malloc(sizeof(ASFDataPacket));
	BYTE sequ_type = 0;
	BYTE padd_type = 0;
	BYTE pack_type = 0;
	BYTE repdatalen_type = 0;
	BYTE mediaobjoffset_type = 0;
	BYTE mediaobjlen_type = 0;
	memset(ret, 0x00, sizeof(ASFDataPacket));

#ifdef _DEBUG
	p_WF->PacketCount++;
	printf("Reading packet from file pos %i (# %i)....\n", ftell(f_WMA), p_WF->PacketCount);
#endif
	fread(ret, 1, 1, f_WMA);

	if ((ret->flags & 0x80) == 0x80) // If the error correction bit is set:
	{
		BYTE errcorr_len = (ret->flags & 0x0F);
		//BYTE errcorr_type = (ret->flags & 0x60);

		ret->errcorr = (BYTE *)malloc(errcorr_len);
		fread(ret->errcorr, errcorr_len, 1, f_WMA);
	}

	if ((ret->flags & 0x10) == 0x10)
	{
		return ret;
	}

	ret->parse_info = (ASFPayloadParseInfo *)malloc(sizeof(ASFPayloadParseInfo));
	memset(ret->parse_info, 0x00, sizeof(ASFPayloadParseInfo));

	fread(ret->parse_info, 2, 1, f_WMA);

	sequ_type = (ret->parse_info->length_type_flags & 0x06) >> 1;
	padd_type = (ret->parse_info->length_type_flags & 0x18) >> 3;
	pack_type = (ret->parse_info->length_type_flags & 0x60) >> 5;

	if (sequ_type == 3)
		sequ_type = 4;
	fread(&ret->parse_info->sequence, sequ_type, 1, f_WMA);

	if (padd_type == 3)
		padd_type = 4;
	fread(&ret->parse_info->padding_length, padd_type, 1, f_WMA);

	if (pack_type == 3)
		pack_type = 4;
	fread(&ret->parse_info->packet_length, pack_type, 1, f_WMA);

	fread(&ret->parse_info->send_time, sizeof(WORD), 3, f_WMA);

	if ((ret->parse_info->length_type_flags & 0x01) == 0) // Single or Multi Payloads?
	{
		ret->single_payload = (ASFSinglePayload *)malloc(sizeof(ASFSinglePayload));
		memset(ret->single_payload, 0x00, sizeof(ASFSinglePayload));

		fread(ret->single_payload, 1, 1, f_WMA);

		repdatalen_type = (ret->parse_info->prop_flags & 0x03) >> 0;
		mediaobjoffset_type = (ret->parse_info->prop_flags & 0x0C) >> 2;
		mediaobjlen_type = (ret->parse_info->prop_flags & 0x30) >> 4;

		if (mediaobjlen_type == 3)
			mediaobjlen_type = 4;
		fread(&ret->single_payload->media_object_no, mediaobjlen_type, 1, f_WMA);

		if (mediaobjoffset_type == 0 || mediaobjoffset_type == 3)
			mediaobjoffset_type = 4;
		fread(&ret->single_payload->media_object_offset, mediaobjoffset_type, 1, f_WMA);

		if (repdatalen_type == 0 || repdatalen_type == 3)
			repdatalen_type = 4;
		fread(&ret->single_payload->rep_data_len, repdatalen_type, 1, f_WMA);

		if (ret->single_payload->rep_data_len == 1) // Compressed?
		{
			/*ret->single_payload->compressed = (ASFSingleComprPayload *)malloc(sizeof(ASFSingleComprPayload));
			memset(ret->single_payload->compressed, 0x00, sizeof(ASFSingleComprPayload));

			fread(ret->single_payload->compressed, 1, 1, f_WMA);

			ret->single_payload->compressed->sub_payload = (BYTE *)malloc();
			fread(ret->single_payload->compressed->sub_payload, pktsize, 1, f_WMA);*/
		}
		else
		{
			DWORD pktsize = 0;
			ret->single_payload->uncompressed = (ASFSingleUnComprPayload *)malloc(sizeof(ASFSingleUnComprPayload));
			memset(ret->single_payload->uncompressed, 0x00, sizeof(ASFSingleUnComprPayload));

			ret->single_payload->uncompressed->rep_data = (BYTE *)malloc(ret->single_payload->rep_data_len);
			fread(ret->single_payload->uncompressed->rep_data, ret->single_payload->rep_data_len, 1, f_WMA);
			pktsize = *((DWORD *)ret->single_payload->uncompressed->rep_data);

			ret->single_payload->uncompressed->payload = (BYTE *)malloc(pktsize);
			fread(ret->single_payload->uncompressed->payload, pktsize, 1, f_WMA);
		}

		fseek(f_WMA, ret->parse_info->padding_length, SEEK_CUR);
	}
	/*else
	{
		BYTE payloadlen_type = 0;
		ret->multi_payload = (ASFMultiPayload *)malloc(sizeof(ASFMultiPayload));
		memset(ret->multi_payload, 0x00, sizeof(ASFMultiPayload));

		fread(ret->multi_payload, 1, 1, f_WMA);

		payloadlen_type = (ret->multi_payload->payload_flags & 0xC0) >> 6;

		repdatalen_type = (ret->parse_info->prop_flags & 0x03);
		mediaobjoffset_type = (ret->parse_info->prop_flags & 0x0C) >> 2;
		mediaobjlen_type = (ret->parse_info->prop_flags & 0x30) >> 4;

		ret->multi_payload->uncompressed = (ASFMultiUnComprPayload *)malloc(sizeof(ASFMultiUnComprPayload));
		memset(ret->multi_payload->uncompressed, 0x00, sizeof(ASFMultiUnComprPayload));

		if (mediaobjlen_type < 3)
			fread(&ret->multi_payload->uncompressed->media_object_no, mediaobjlen_type, 1, f_WMA);
		else
			fread(&ret->multi_payload->uncompressed->media_object_no, 4, 1, f_WMA);

		if (mediaobjoffset_type < 3)
			fread(&ret->multi_payload->uncompressed->media_object_offset, mediaobjoffset_type, 1, f_WMA);
		else
			fread(&ret->multi_payload->uncompressed->media_object_offset, 4, 1, f_WMA);

		if (repdatalen_type < 3)
			fread(&ret->multi_payload->uncompressed->rep_data_len, repdatalen_type, 1, f_WMA);
		else
			fread(&ret->multi_payload->uncompressed->rep_data_len, 4, 1, f_WMA);

		ret->multi_payload->uncompressed->rep_data = (BYTE *)malloc(ret->multi_payload->uncompressed->rep_data_len);
		fread(ret->multi_payload->uncompressed->rep_data, ret->multi_payload->uncompressed->rep_data_len, 1, f_WMA);

		if (payloadlen_type == 0)
			fread(&ret->multi_payload->uncompressed->payload_len, 2, 1, f_WMA);
		else if (payloadlen_type < 3)
			fread(&ret->multi_payload->uncompressed->payload_len, payloadlen_type, 1, f_WMA);
		else
			fread(&ret->multi_payload->uncompressed->payload_len, 4, 1, f_WMA);

		ret->multi_payload->uncompressed->payload = (BYTE *)malloc(ret->multi_payload->uncompressed->payload_len);
		fread(ret->multi_payload->uncompressed->payload, ret->multi_payload->uncompressed->payload_len, 1, f_WMA);
	}*/

	return ret;
}

void DestroyPacket(ASFDataPacket **Packet)
{
	ASFDataPacket *pkt = *Packet;

	if (pkt != NULL)
	{
		if (pkt->single_payload != NULL)
		{
			if (pkt->single_payload->rep_data_len == 1)
			{
				if (pkt->single_payload->compressed != NULL)
				{
					free(pkt->single_payload->compressed->sub_payload);
					free(pkt->single_payload->compressed);
				}
			}
			else
			{
				if (pkt->single_payload->uncompressed != NULL)
				{
					free(pkt->single_payload->uncompressed->payload);
					free(pkt->single_payload->uncompressed->rep_data);
					free(pkt->single_payload->uncompressed);
				}
			}

			free(pkt->single_payload);
		}

		if (pkt->parse_info != NULL)
		{
			free(pkt->parse_info);
		}

		free(pkt);
	}

	*Packet = NULL;
}

int main(int argc, char **argv)
{
	std::string oname;
	WMA_Intern *InFile = NULL;
	FILE *f_Out = NULL;
	FILE *f_Meta = NULL;

	if (argc != 2)
		return -1;

	InFile = (WMA_Intern *)wmaOpenR(argv[1]);
	oname = std::string(argv[1]);
	oname.append(".audio");
	f_Out = fopen(oname.c_str(), "wb");
	oname.clear();
	oname = std::string(argv[1]);
	oname.append(".meta");
	f_Meta = fopen(oname.c_str(), "wb");
	oname.clear();

	fwrite(InFile->p_ASFStrm[0], sizeof(ASFStreamHeader) - (2 * sizeof(BYTE *)), 1, f_Meta);
	fwrite(InFile->p_ASFStrm[0]->data, InFile->p_ASFStrm[0]->data_len, 1, f_Meta);
	fwrite(InFile->p_ASFStrm[0]->errcorr, InFile->p_ASFStrm[0]->errcorr_len, 1, f_Meta);

	for (int i = 0; i < InFile->p_ASFMain->data_packets; i++)
	{
		BYTE *buf;
		DWORD bufsize;
		ASFDataPacket *pkt = ReadPacket(InFile);

		if (pkt->multi_payload == NULL)
		{
			if (pkt->single_payload->compressed == NULL)
			{
				buf = pkt->single_payload->uncompressed->payload;
				bufsize = *((DWORD *)pkt->single_payload->uncompressed->rep_data);
			}
			else
			{
				buf = pkt->single_payload->compressed->sub_payload;
				//bufsize = pkt->single_payload->
				bufsize = 0;
			}
		}
		else
		{
			if (pkt->multi_payload->uncompressed != NULL)
			{
				buf = pkt->multi_payload->uncompressed->payload;
				bufsize = *((int *)pkt->multi_payload->uncompressed->rep_data);
			}
			else
			{
				buf = NULL;
				bufsize = 0;
			}
		}

		if (buf != NULL)
			fwrite(buf, bufsize, 1, f_Out);

		DestroyPacket(&pkt);
	}

	fclose(f_Out);
	return 0;
}

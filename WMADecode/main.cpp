#define _CRT_SECURE_NO_WARNINGS
#include <vector>
#include <string>
#include <windows.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifndef QWORD
typedef unsigned __int64 QWORD;
#endif

#include "../libAudio/WMA_HuffTables.h"

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

typedef struct _BitStreamReader
{
	BYTE *Data;
	DWORD nBytesRead;
	DWORD nBitsRead;
	DWORD nDataLen;
	BYTE bData;
} BitStreamReader;

#define BLOCK_NB_MAX	11
#define BLOCK_NB_MIN	7
#define BLOCK_MAX_SIZE	2048
#define BLOCK_NB_SIZES	5

#define HB_MAX_SIZE		16

#define MAX_CODED_SUPERFRAME_SIZE	16384

#define MAX_CHANNELS	2

struct _MDCTContext;

typedef struct _FFTComplex
{
    float re, im;
} FFTComplex;

typedef struct _FFTContext
{
    int nbits;
    int inverse;
    USHORT *revtab;
    FFTComplex *exptab;
} FFTContext;

typedef struct _MDCTContext
{
    int n; /* size of MDCT (i.e. number of input data * 2) */
    int nbits; /* n = 2^nbits */
    /* pre/post rotation tables */
    float *tcos;
    float *tsin;
    FFTContext fft;
} MDCTContext;

typedef struct _WMA_Intern
{
	FILE *f_WMA, *f_Out;
	FileInfo *p_FI;
	std::vector<ASFStreamHeader *> p_ASFStrm;
	BitStreamReader *p_Reader;
	BYTE WMAVersion;
	DWORD exp_sizes[BLOCK_NB_SIZES];
	WORD exp_bands[BLOCK_NB_SIZES][25];
	DWORD hb_start[BLOCK_NB_SIZES];
	int coefs_start;
	int coefs_end[BLOCK_NB_SIZES];
	DWORD exp_high_sizes[BLOCK_NB_SIZES];
	DWORD exp_hbs[BLOCK_NB_SIZES][HB_MAX_SIZE];
	DWORD hb_values[MAX_CHANNELS][HB_MAX_SIZE];
	DWORD exps_bsize[MAX_CHANNELS];
	float exps[MAX_CHANNELS][BLOCK_MAX_SIZE];
	float max_exp[MAX_CHANNELS];
	float lsp_costbl[BLOCK_MAX_SIZE];
	float lsp_powetbl[256];
	float lsp_powmtbl1[128];
	float lsp_powmtbl2[128];
	float noise_tbl[8192];
	float noise_mul;
	int noise_idx;
	int flags1;
	int flags2;
	BYTE mdctlen_bits;
	BYTE blocklen_bits;
	DWORD reset_blocklengths;
	BYTE next_blocklen_bits;
	BYTE prev_blocklen_bits;
	DWORD mdctlen; // framelen
	DWORD blocklen;
	DWORD blocknum;
	DWORD blockpos;
	BYTE nb_block_sizes;
	bool noise_coding;
	VLC hgain_vlc;
	VLC exp_vlc;
	bool use_exp_vlc;
	VLC coef_vlc[2];
	WORD *run_tbl[2];
	WORD *lvl_tbl[2];
	WORD *int_tbl[2];
	const CoefVLCTable *coef_vlcs[2];
	short coefs1[MAX_CHANNELS][BLOCK_MAX_SIZE];
	float coefs[MAX_CHANNELS][BLOCK_MAX_SIZE];
	float frame[MAX_CHANNELS][BLOCK_MAX_SIZE * 2];
	float end_frame[MAX_CHANNELS][BLOCK_MAX_SIZE / 2];
	MDCTContext imdct_ctx[BLOCK_NB_SIZES];
	float output[BLOCK_MAX_SIZE * 2];
	float mdcttmp[BLOCK_MAX_SIZE];
    float *windows[BLOCK_NB_SIZES];
	DWORD nFrameLen;
	int byteoffset_bits;
	BYTE lastsuperframe[MAX_CODED_SUPERFRAME_SIZE + 4];
    int lastsuperframe_len;
	int last_bitoffset;
} WMA_Intern;

static const GUID ASF_AUDIOSTREAM_GUID = {0xF8699E40, 0x5B4D, 0x11CF, 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B};

BYTE *pkt = NULL;
DWORD pktlen = 0;

const BYTE log2_table[256] =
{
	0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

void init_bitstream_reader(WMA_Intern *p_WF, BYTE *data, int nDataLen)
{
	if (p_WF->p_Reader == NULL)
		p_WF->p_Reader = (BitStreamReader *)malloc(sizeof(BitStreamReader));
	memset(p_WF->p_Reader, 0x00, sizeof(BitStreamReader));

	p_WF->p_Reader->Data = data;
	p_WF->p_Reader->nBytesRead = 1;
	p_WF->p_Reader->nBitsRead = 0;
	p_WF->p_Reader->bData = data[0];
	p_WF->p_Reader->nDataLen = nDataLen;
}

int ReadNBits(int nBits, BitStreamReader *p_Reader)
{
	BYTE data = p_Reader->bData;
	int tmp = 0, bts = nBits;
	int nBitsRead = p_Reader->nBitsRead;

	if (bts > 32)
		return 0;

	while (bts > 0)
	{
		nBitsRead = p_Reader->nBitsRead;

		if (nBitsRead == 8)
		{
			p_Reader->bData = p_Reader->Data[p_Reader->nBytesRead];
			p_Reader->nBytesRead++;
			p_Reader->nBitsRead = nBitsRead = 0;
		}

		if (bts <= 8)
		{
			if (8 - nBitsRead >= bts)
			{
				tmp <<= bts;

				for (int i = 0; i < bts; i++)
				{
					tmp += data & (1 << i);
				}

				p_Reader->bData >>= bts;
				p_Reader->nBitsRead += bts;
				bts = 0;
			}
			else
			{
				int bits = 8 - nBitsRead;
				tmp <<= bits;

				for (int i = 0; i < bits; i++)
				{
					tmp += data & (1 << i);
				}

				p_Reader->bData >>= bits;
				p_Reader->nBitsRead += bits;
				bts -= bits;
			}
		}
		else
		{
			if (8 - nBitsRead == 8)
			{
				tmp <<= 8;
				tmp += data;

				p_Reader->bData >>= 8;
				p_Reader->nBitsRead += 8;
				bts -= 8;
			}
			else
			{
				int bits = 8 - nBitsRead;
				tmp <<= bits;

				for (int i = 0; i < bits; i++)
				{
					tmp += data & (1 << i);
				}

				p_Reader->bData >>= bits;
				p_Reader->nBitsRead += bits;
				bts -= bits;
			}
		}
	}
	return tmp;
}

void AlignBits(BitStreamReader *p_Reader)
{
	int n = 8 - (int)p_Reader->nBitsRead;
	if (n > 0 && n < 8)
	{
		p_Reader->bData = p_Reader->Data[p_Reader->nBytesRead];
		p_Reader->nBitsRead = 0;
		p_Reader->nBytesRead++;
	}
}

void SkipNBits(int bits, BitStreamReader *p_Reader)
{
	int bit = bits;

	while (bit > 0)
	{
		if (p_Reader->nBitsRead + bit <= 8)
		{
			p_Reader->nBitsRead += bit;
			p_Reader->bData >>= bit;
			bit = 0;
		}
		else
		{
			p_Reader->bData = p_Reader->Data[p_Reader->nBytesRead];
			bit -= 8 - p_Reader->nBitsRead;
			p_Reader->nBitsRead = 0;
			p_Reader->nBytesRead++;
		}
	}
}

DWORD ReadVLC(BitStreamReader *p_Reader, WORD (*table)[2], int bits, int max_depth)
{
	DWORD code, index, nb_bits;
	short bit;

	//index = ((UINT)p_Reader->bData) >> (32 - bits);
	index = ReadNBits(bits, p_Reader);
	code = table[index][0];
	bit = table[index][1];

	if (max_depth > 1 && bit < 0)
	{
		SkipNBits(bits, p_Reader);
		nb_bits = -bit;
		//index = (((UINT)p_Reader->bData) >> (32 - nb_bits)) + code;
		index = ReadNBits(nb_bits, p_Reader);// + code;
		code = table[index][0];
		bit = table[index][1];

		if (max_depth > 2 && bit < 0)
		{
			SkipNBits(bits, p_Reader);
			nb_bits = -bit;

			//index = (((UINT)p_Reader->bData) >> (32 - nb_bits)) + code;
			index = ReadNBits(nb_bits, p_Reader);// + code;
			code = table[index][0];
			bit = table[index][1];
		}
	}
	SkipNBits(bit, p_Reader);

	return code;
}

int ReadExpVLC(int chan, WMA_Intern *p_WF)
{
	const WORD *band_ptr = p_WF->exp_bands[p_WF->mdctlen_bits - p_WF->blocklen_bits];
	const WORD *ptr = band_ptr;
	int n, last_exp, code;
	float *exps = p_WF->exps[chan];
	float *exps_end = exps + p_WF->blocklen;
	float max_scale;

	if (p_WF->WMAVersion == 1)
	{
		last_exp = ReadNBits(5, p_WF->p_Reader) + 10;
		max_scale = pow(10, last_exp * (1.0F / 16.0F));
		n = *ptr;
		ptr++;

		do
		{
			*exps = max_scale;
			exps++;
			n--;
		}
		while (n > 0);
	}
	else
		last_exp = 36;

	while (exps < exps_end)
	{
		float v;
		code = ReadVLC(p_WF->p_Reader, p_WF->exp_vlc.table, 8, 3);
		if (code < 0)
			return -1;

		last_exp += code - 60;

		v = pow(10, last_exp * (1.0F / 16.0F));
		if (v > max_scale)
			max_scale = v;
		n = *ptr;
		ptr++;

		do
		{
			*exps = max_scale;
			exps++;
			n--;
		}
		while (n > 0);
	}

	p_WF->max_exp[chan] = max_scale;
	return 0;
}

inline float pow_m1_4(WMA_Intern *p_WF, float x)
{
	union
	{
		float x;
		DWORD y;
	} val1, val2;
	float a, b;

	val1.x = x;
	val2.y = ((val1.y << 7) & ((1 << 23) - 1)) | (127 << 23);
	a = p_WF->lsp_powmtbl1[(val1.y >> 16) & 127];
	b = p_WF->lsp_powmtbl2[(val1.y >> 16) & 127];

	return p_WF->lsp_powetbl[val1.y >> 23] * (a + (b * val2.x));
}

void lsp_curve(WMA_Intern *p_WF, float *out, float *max_ptr, int blocklen, float *lsp)
{
	int i;
	float val_max = 0, p, q, v;

	for (i = 0; i < blocklen; i++)
	{
		int j;
		p = q = 0.5F;

		for (j = 1; j < 10; j += 2)
		{
			q *= p_WF->lsp_costbl[i] - lsp[j - 1];
			p *= p_WF->lsp_costbl[i] - lsp[j];
		}

		p *= p * (2.0F - p_WF->lsp_costbl[i]);
		q *= q * (2.0F + p_WF->lsp_costbl[i]);
		v = pow_m1_4(p_WF, p + q);
		if (v > val_max)
			val_max = v;
		out[i] = v;
	}

	*max_ptr = val_max;
}

void ReadExpLSP(int chan, WMA_Intern *p_WF)
{
	float lsp_coefs[10];
	int i, val;
	BitStreamReader *p_Reader = p_WF->p_Reader;

	for (i = 0; i < 10; i++)
	{
		if (i == 0 || i >= 8)
			val = ReadNBits(3, p_Reader);
		else
			val = ReadNBits(4, p_Reader);
		lsp_coefs[i] = lsp_codebook[i][val];
	}

	lsp_curve(p_WF, p_WF->exps[chan], &p_WF->max_exp[chan], p_WF->blocklen, lsp_coefs);
}

DWORD alloc_table(VLC *vlc, int size)
{
	int index = vlc->table_size;
	vlc->table_size += size;

	if (vlc->table_size > vlc->table_allocated)
	{
		vlc->table_allocated += (1 << vlc->bits);
		vlc->table = (WORD (*)[2])realloc(vlc->table, 4 * vlc->table_allocated);

		if (vlc->table == NULL)
			return -1;
	}
	return index;
}

#define GET_DATA(v, data, pos, size)\
{\
	BYTE *ptr = (BYTE *)data + pos;\
	switch (size)\
	{\
		case 1:\
			v = *(BYTE *)ptr;\
			break;\
		case 2:\
			v = *(WORD *)ptr;\
			break;\
		case 4:\
			v = *(DWORD *)ptr;\
			break;\
	}\
}

DWORD build_table(VLC *vlc, int tbl_bits, int nb_codes, const BYTE *bits, int bits_wrap, void *codes, int codes_wrap,
				  int codes_size, DWORD *symbols, int symbols_wrap, DWORD code_prefix, int n_prefix)
{	
	int j, l, symbol, prefix, table_size, table_index, index;
	WORD (*table)[2];
	WORD bit;
	DWORD code;

	table_size = 1 << tbl_bits;
	table_index = alloc_table(vlc, table_size);
	if (table_index < 0)
		return -1;

	table = &vlc->table[table_index];

	for (j = 0; j < table_size; j++)
	{
		table[j][1] = 0;
		table[j][0] = -1;
	}

	// First pass
	for (j = 0; j < nb_codes; j++)
	{
		int k;
		bit = *(bits + j);// * bits_wrap);
		GET_DATA(code, codes, j * codes_wrap, codes_size);

		if (bit <= 0)
			continue;
		symbol = j;

		bit -= n_prefix;
		prefix = code >> bit;

		if (bit > 0 && prefix == code_prefix)
		{
			if (bit <= tbl_bits)
			{
				k = (code << (tbl_bits - bit)) & (table_size - 1);
				for (l = 0; l < (1 << (tbl_bits - bit)); l++)
				{
					if (table[k][1] != 0)
						return -1;
					table[k][1] = bit;
					table[k][0] = symbol;
					k++;
				}
			}
			else
			{
				int bit1;
				bit -= tbl_bits;
				k = (code >> bit) & ((1 << tbl_bits) - 1);
				bit1 = -table[j][1];
				if (bit > bit1)
					bit1 = bit;
				table[j][1] = -bit1;
			}
		}
	}

	// second pass
	for (j = 0; j < table_size; j++)
	{
		bit = table[j][1];

		if (bit < 0)
		{
			bit = -bit;
			if (bit > tbl_bits)
			{
				bit = tbl_bits;
				table[j][1] = -bit;
			}
			index = build_table(vlc, bit, nb_codes, bits, bits_wrap, codes, codes_wrap, codes_size, symbols, symbols_wrap,
				((code_prefix << tbl_bits) | j), n_prefix + tbl_bits);

			if (index < 0)
				return -1;
			table = &vlc->table[table_index];
			table[j][0] = index;
		}
	}

	return table_index;
}

int init_vlc(VLC *vlc, int nb_bits, int nb_codes, const BYTE *bits, int bits_wrap, void *codes, int codes_wrap,
				  int codes_size, DWORD *symbols, int symbols_wrap)
{
	vlc->bits = nb_bits;
	vlc->table = NULL;
	vlc->table_allocated = 0;
	vlc->table_size = 0;

	if (build_table(vlc, nb_bits, nb_codes, bits, bits_wrap, codes, codes_wrap, codes_size, symbols, symbols_wrap, 0, 0) < 0)
	{
		free(vlc->table);
		return -1;
	}
	return 0;
}

void init_coef_vlc(VLC *vlc, WORD **p_run_tbl, WORD **p_lvl_tbl, WORD **p_int_tbl, const CoefVLCTable *vlc_tbl)
{
	int nb_bits = vlc_tbl->n;
	const BYTE *tbl_bits = vlc_tbl->huffbits;
	const DWORD *tbl_codes = vlc_tbl->huffcodes;
	const WORD *lvls_tbl = vlc_tbl->levels;
	WORD *run_tbl, *lvl_tbl, *int_tbl;
	int i = 2, k = 0, lvl = 1;

	init_vlc(vlc, 9, nb_bits, tbl_bits, 1, (void *)tbl_codes, 4, 4, NULL, 0);

	run_tbl = (WORD *)malloc(nb_bits * 2);
	lvl_tbl = (WORD *)malloc(nb_bits * 2);
	int_tbl = (WORD *)malloc(nb_bits * 2);

	while (i < nb_bits)
	{
		int l, j = 0;

		int_tbl[k] = i;
		l = lvls_tbl[k];
		k++;

		for (j = 0; j < l; j++)
		{
			run_tbl[i] = j;
			lvl_tbl[i] = lvl;
			i++;
		}

		lvl++;
	}

	*p_run_tbl = run_tbl;
	*p_lvl_tbl = lvl_tbl;
	*p_int_tbl = int_tbl;
}

inline int log2(UINT v)
{
	int n = 0;

	if ((v & 0xffff0000) != 0)
	{
		v >>= 16;
		n += 16;
	}
	if ((v & 0xff00) != 0)
	{
		v >>= 8;
		n += 8;
	}
	n += log2_table[v];

	return n;
}

int total_gain_to_bits(int total_gain)
{
	if (total_gain < 15)
		return 13;
	else if (total_gain < 32)
		return 12;
	else if (total_gain < 40)
		return 11;
	else if (total_gain < 45)
		return 10;
	else
		return  9;
}

FileInfo *WMA_GetFileInfo(WMA_Intern *p_WF)
{
	FileInfo *ret = (FileInfo *)malloc(sizeof(FileInfo));
	memset(ret, 0x00, sizeof(FileInfo));
	p_WF->p_FI = ret;

	for (DWORD i = 0; i < p_WF->p_ASFStrm.size(); i++)
	{
		ASFStreamHeader *strm = p_WF->p_ASFStrm[0];

		if (strm->strm_guid == ASF_AUDIOSTREAM_GUID)
		{
			float bps, bps1;
			int bit_rate;
			int sample_rate;
			float hf;

			if (strm->data_len == 14)
			{
				WAVEFORMAT *data = (WAVEFORMAT *)strm->data;
				ret->Channels = data->nChannels;
				ret->BitsPerSample = 8;
				ret->BitRate = data->nSamplesPerSec;
				bit_rate = data->nAvgBytesPerSec * 8;
				p_WF->WMAVersion = (data->wFormatTag == 0x160 ? 1 : 2);
			}
			else if (strm->data_len >= 18)
			{
				WAVEFORMATEX *data = (WAVEFORMATEX *)strm->data;

				ret->Channels = data->nChannels;
				ret->BitsPerSample = data->wBitsPerSample;
				ret->BitRate = data->nSamplesPerSec;
				bit_rate = data->nAvgBytesPerSec * 8;
				p_WF->WMAVersion = (data->wFormatTag == 0x160 ? 1 : 2);

				if (p_WF->WMAVersion == 1)
				{
					p_WF->flags1 = ((WORD *)data)[9];
					p_WF->flags2 = ((WORD *)data)[10];
					p_WF->coefs_start = 3;
				}
				else
				{
					p_WF->flags1 = ((WORD *)data)[9] << 16;
					p_WF->flags1 += ((WORD *)data)[10];
					p_WF->flags2 = ((WORD *)data)[11];
					p_WF->coefs_start = 0;
				}
			}

			p_WF->mdctlen_bits = (p_WF->p_FI->BitRate <= 16000 ? 9 : (p_WF->p_FI->BitRate <= 22050 ||
				(p_WF->p_FI->BitRate <= 32000 && p_WF->WMAVersion == 1) ? 10 : 11));
			p_WF->mdctlen = 1 << p_WF->mdctlen_bits;
			p_WF->blocklen_bits = p_WF->mdctlen_bits;
			p_WF->blocklen = p_WF->mdctlen;

			if ((p_WF->flags2 & 4) == 8)
			{
				int nb_max;
				int nb = ((p_WF->flags2 >> 3) & 3) + 1;
				if (ret->BitRate / ret->Channels >= 32000)
					nb += 2;
				nb_max = p_WF->mdctlen_bits - BLOCK_NB_MIN;
				if (nb > nb_max)
					nb = nb_max;
				p_WF->nb_block_sizes = nb + 1;
			}
			else
				p_WF->nb_block_sizes = 1;

			p_WF->noise_coding = true;

			if (p_WF->WMAVersion == 2)
			{
				if (ret->BitRate >= 44100)
					sample_rate = 44100;
				else if (ret->BitRate >= 22050)
					sample_rate = 22050;
				else if (ret->BitRate >= 16000)
					sample_rate = 16000;
				else if (ret->BitRate >= 11025)
					sample_rate = 11025;
				else if (ret->BitRate >= 8000)
					sample_rate = 8000;
				else
					sample_rate = ret->BitRate;
			}
			else
				sample_rate = ret->BitRate;

			bps = (float)bit_rate / (float)(ret->Channels * ret->BitRate);
			bps1 = bps;
			hf = (float)ret->BitRate / 2.0F;
			p_WF->byteoffset_bits = log2((int)(bps * p_WF->mdctlen / 8.0 + 0.5)) + 2;

			if (ret->Channels == 2)
				bps1 *= 1.6F;
			switch (sample_rate)
			{
				case 44100:
				{
					if (bps1 >= 0.61F)
						p_WF->noise_coding = false;
					else
						hf /= 2.5F;
					break;
				}
				case 22050:
				{
					if (bps1 >= 1.16F)
						p_WF->noise_coding = false;
					else if (bps >= 0.72F)
						hf *= 0.7F;
					else
						hf *= 0.6F;
					break;
				}
				case 16000:
				{
					if (bps > 0.5F)
						hf /= 2;
					else
						hf *= 0.3F;
					break;
				}
				case 11025:
				{
					hf *= 0.7F;
					break;
				}
				case 8000:
				{
					if (bps <= 0.625F)
						hf /= 2;
					else if (bps > 0.75F)
						p_WF->noise_coding = false;
					else
						hf *= 0.65F;
					break;
				}
				default:
				{
					if (bps >= 0.8F)
						hf *= 0.75F;
					else if (bps >= 0.6F)
						hf *= 0.6F;
					else
						hf /= 2;
					break;
				}
			}

			for (int k = 0; k < p_WF->nb_block_sizes; k++)
			{
				int block_len = p_WF->mdctlen >> k;
				int lpos, pos, a, b, l;
				DWORD j;
				const BYTE *table;

				if (p_WF->WMAVersion == 1)
				{
					lpos = 0;

					for (j = 0; j < 25; j++)
					{
						a = wma_critical_freqs[j];
						b = ret->BitRate;
						pos = ((block_len * 2 * a) + (b >> 1)) / b;
						if (pos > block_len)
							pos = block_len;
						p_WF->exp_bands[0][i] = pos - lpos;
						if (pos >= block_len)
						{
							i++;
							break;
						}
						lpos = pos;
					}
					p_WF->exp_sizes[0] = j;
				}
				else
				{
					table = NULL;
					a = p_WF->mdctlen_bits - BLOCK_NB_MIN - k;
					if (a < 3)
					{
						if (ret->BitRate >= 44100)
							table = exponent_band_44100[a];
						else if (ret->BitRate >= 32000)
							table = exponent_band_32000[a];
						else if (ret->BitRate >= 22050)
							table = exponent_band_22050[a];
					}
					if (table != NULL)
					{
						for (j = 0; j < table[0]; j++)
							p_WF->exp_bands[k][j] = table[j];
						p_WF->exp_sizes[k] = table[0];
						table++;
					}
					else
					{
						l = 0;
						lpos = 0;
						for (j = 0; j < 25; j++)
						{
							a = wma_critical_freqs[j];
							b = ret->BitRate;
							pos = ((block_len * 2 * a) + (b << 1)) / (4 * b);
							pos <<= 2;
							if (pos > block_len)
								pos = block_len;
							if (pos > lpos)
							{
								p_WF->exp_bands[k][l] = pos - lpos;
								l++;
							}
							if (pos >= block_len)
								break;
							lpos = pos;
						}
						p_WF->exp_sizes[k] = l;
					}
				}

				p_WF->coefs_end[k] = (p_WF->mdctlen - ((p_WF->mdctlen * 9) / 100)) >> k;
				p_WF->hb_start[k] = (int)((block_len * 2 * hf) / ret->BitRate + 0.5);

				pos = 0;
				l = 0;
				for (j = 0; j < p_WF->exp_sizes[k]; j++)
				{
					DWORD start = pos, end;
					pos += p_WF->exp_bands[k][j];
					end = pos;
					if (start < p_WF->hb_start[k])
						start = p_WF->hb_start[k];
					if (end > (DWORD)p_WF->coefs_end[k])
						end = (DWORD)p_WF->coefs_end[k];
					if (end > start)
					{
						p_WF->exp_hbs[k][l] = end - start;
						l++;
					}
				}
				p_WF->exp_high_sizes[k] = l;
			}

			if (p_WF->noise_coding == true)
			{
				init_vlc(&p_WF->hgain_vlc, 9, sizeof(hgain_huffbits), hgain_huffbits, 1, (void *)hgain_huffcodes, 2, 2, NULL, 0);
			}

			if ((p_WF->flags2 & 1) == 1)
			{
				p_WF->use_exp_vlc = true;
				init_vlc(&p_WF->exp_vlc, 8, sizeof(scale_huffbits), scale_huffbits, 1, (void *)scale_huffcodes, 4, 4, NULL, 0);
			}
			else
			{
				float flen_pi = (float)M_PI / p_WF->mdctlen, b = 1.0F;
				DWORD i;

				for (i = 0; i < p_WF->mdctlen; i++)
					p_WF->lsp_costbl[i] = 2.0F * cos(flen_pi * i);

				for (i = 0; i < 256; i++)
					p_WF->lsp_powetbl[i] = pow(2.0F, (i - 126) * -0.25F);

				for (i = 127; i >= 0; i--)
				{
					float a = pow((float)(128 + i) * (0.5F / 128.0F), -0.25F);
					p_WF->lsp_powmtbl1[i] = 2 * a - b;
					p_WF->lsp_powmtbl2[i] = a - b;
					a = b;
				}
			}

			for (i = 0; i < p_WF->nb_block_sizes; i++)
			{
				int n = 1 << (p_WF->mdctlen_bits - i), j;
				float *window = (float *)malloc(sizeof(float) * n);
				double alpha = M_PI / (2.0 * n);
				for (j = 0; j < n; j++)
					window[j] = (float)sin((j + 0.5) * alpha);
				p_WF->windows[i] = window;
			}

			p_WF->reset_blocklengths = 1;
			if (p_WF->noise_coding == true)
			{
				int i;
				DWORD seed = 1;
				float norm;

				p_WF->noise_mul = (p_WF->use_exp_vlc == true ? 0.02F : 0.04F);
				norm = (1.0F / (float)(1 << 31)) * sqrt(3.0F) * p_WF->noise_mul;
				for (i = 0; i < 8192; i++)
				{
					seed = seed * 314159 + 1;
					p_WF->noise_tbl[i] = (float)seed * norm;
				}
			}

			{
				int cvt = 2;

				if (ret->BitRate >= 32000)
				{
					if (bps1 < 0.72F)
						cvt = 0;
					else if (bps1 < 1.16F)
						cvt = 1;
				}

				p_WF->coef_vlcs[0] = &coef_vlcs[cvt * 2];
				p_WF->coef_vlcs[1] = &coef_vlcs[cvt * 2 + 1];

				init_coef_vlc(&p_WF->coef_vlc[0], &p_WF->run_tbl[0], &p_WF->lvl_tbl[0], &p_WF->int_tbl[0], p_WF->coef_vlcs[0]);
				init_coef_vlc(&p_WF->coef_vlc[1], &p_WF->run_tbl[1], &p_WF->lvl_tbl[1], &p_WF->int_tbl[1], p_WF->coef_vlcs[1]);
			}
		}
	}

	return ret;
}

int mdct_init(MDCTContext *ctx, int nbits, int inverse)
{
	FFTContext *fft = &ctx->fft;
	int n = 1 << nbits, n4 = n >> 2, i;
	double alpha, s2, c, s;

	memset(ctx, 0x00, sizeof(MDCTContext));
	ctx->nbits = nbits;
	ctx->n = n;
	ctx->tcos = (float *)malloc(n4 * sizeof(float));
	if (ctx->tcos == NULL)
		goto fail;
	ctx->tsin = (float *)malloc(n4 * sizeof(float));
	if (ctx->tsin == NULL)
		goto fail;

	for (i = 0; i < n4; i++)
	{
		alpha = 2 * M_PI * (i + 1.0 / 8.0) / n;
		ctx->tcos[i] = (float)(-cos(alpha));
		ctx->tsin[i] = (float)(-sin(alpha));
	}

	nbits -= 2;
	fft->nbits = nbits;
	n = 1 << nbits;

	fft->exptab = (FFTComplex *)malloc((n / 2) * sizeof(FFTComplex));
	if (fft->exptab == NULL)
		goto fail;
	fft->revtab = (USHORT *)malloc(n * sizeof(USHORT));
	if (fft->revtab == NULL)
		goto fail;

	fft->inverse = inverse;
	s2 = inverse != 0 ? 1.0 : -1.0;

	for (i = 0; i < (n / 2); i++)
	{
		alpha = 2 * M_PI * i / n;
		c = cos(alpha);
		s = sin(alpha) * s2;
		fft->exptab[i].re = (float)c;
		fft->exptab[i].im = (float)s;
	}

	for (i = 0; i < n; i++)
	{
		int m = 0, j;
		for (j = 0; j < nbits; j++)
			m |= ((i >> j) & 1) << (nbits - j - 1);
		fft->revtab[i] = m;
	}

	return 0;
fail:
	free(ctx->fft.revtab);
	free(ctx->fft.exptab);
	free(ctx->tcos);
	free(ctx->tcos);
	return -1;
}

/* complex multiplication: p = a * b */
#define CMUL(pre, pim, are, aim, bre, bim) \
{\
    (pre) = (are) * (bre) - (aim) * (bim);\
    (pim) = (are) * (bim) + (aim) * (bre);\
}

/* butter fly op */
#define BF(pre, pim, qre, qim, pre1, pim1, qre1, qim1) \
{\
  pre = (pre1 + qre1);\
  pim = (pim1 + qim1);\
  qre = (pre1 - qre1);\
  qim = (pim1 - qim1);\
}

void fft(FFTContext *fft, FFTComplex *tmp)
{
	int ln = fft->nbits, np = 1 << ln, j, np2;
	register FFTComplex *p, *q;
	int nblocks, nloops;
	FFTComplex *exptab = fft->exptab;

	// Pass 0
	p = tmp;
	j = (np >> 1);
	do
	{
		BF(p[0].re, p[0].im, p[1].re, p[1].im,
			p[0].re, p[0].im, p[1].re, p[1].im);
		p += 2;
		j--;
	} while (j >= 0);

	// Pass 1
	p = tmp;
	j = (np >> 2);
	if (fft->inverse != 0)
	{
		do
		{
			BF(p[0].re, p[0].im, p[2].re, p[2].im,
				p[0].re, p[0].im, p[2].re, p[2].im);
			BF(p[1].re, p[1].im, p[3].re, p[3].im,
				p[1].re, p[1].im, -p[3].re, p[3].im);
			p += 4;
			j--;
		} while (j >= 0);
	}
	else
	{
		do
		{
			BF(p[0].re, p[0].im, p[2].re, p[2].im,
				p[0].re, p[0].im, p[2].re, p[2].im);
			BF(p[1].re, p[1].im, p[3].re, p[3].im,
				p[1].re, p[1].im, p[3].re, -p[3].im);
			p += 4;
			j--;
		} while (j >= 0);
	}

	// pass 2
	nblocks = np >> 3;
	nloops = 1 << 2;
	np2 = np >> 1;
	do
	{
		p = tmp;
		q = tmp + nloops;
		for (j = 0; j < nblocks; j++)
		{
			int l;
			BF(p->re, p->im, q->re, q->im,
				p->re, p->im, q->re, q->im);
			p++;
			q++;
			for (l = nblocks; l < np2; l += nblocks)
			{
			    float tmp_re, tmp_im;
				CMUL(tmp_re, tmp_im, exptab[l].re, exptab[l].im, q->re, q->im);
				BF(p->re, p->im, q->re, q->im, p->re, p->im, tmp_re, tmp_im);
				p++;
				q++;
			}
			p += nloops;
			q += nloops;
		}
		nblocks >>= 1;
		nloops <<= 1;
	} while (nblocks != 0);
}

void imdct(MDCTContext *ctx, float *output, const float *input, FFTComplex *tmp)
{
	int n = 1 << ctx->nbits, k, j;
	int n2 = n >> 1, n4 = n >> 2, n8 = n >> 3;
	const float *in1 = input, *in2 = input + n2 - 1;
	const USHORT *revtab = ctx->fft.revtab;
	float *tcos = ctx->tcos, *tsin = ctx->tsin;

	// pre-rotation
	for (k = 0; k < n4; k++)
	{
		j = revtab[k];
		CMUL(tmp[j].re, tmp[j].im, *in2, *in1, tcos[k], tsin[k]);
		in1 += 2;
		in2 -= 2;
	}
	// calc/rotation
	fft(&ctx->fft, tmp);

	// post-rotation/reordering
	for (k = 0; k < n4; k++)
		CMUL(tmp[k].re, tmp[k].im, tmp[k].re, tmp[k].im, tcos[k], tsin[k]);
	for (k = 0; k < n8; k++)
	{
		output[2 * k] = -tmp[n8 + k].im;
		output[n2 - 1 - (2 * k)] = tmp[n8 + k].im;
		output[2 * k + 1] = tmp[n8 - 1 - k].re;
		output[n2 - 2 - (2 * k)] = -tmp[n8 - 1 - k].re;
		output[n2 + 2  * k] = -tmp[k  + n8].re;
		output[n - 1 - (2 * k)] = -tmp[k + n8].re;
		output[n2 + (2 * k) + 1] = tmp[n8 - k - 1].im;
		output[n - 2 - (2 * k)] = tmp[n8 - k - 1].im;
	}
}

void vector_fmul_add_add(float *dst, const float *src0, const float *src1,
							const float *src2, int src3, int len, int step)
{
	for (int i = 0; i < len; i++)
		dst[i * step] = src0[i] * src1[i] + src2[i] + src3;
}

void vector_fmul_reverse(float *dst, const float *src0, const float *src1,
						   int len)
{
	int i;
	src1 += len - 1;
	for(i = 0; i < len; i++)
		dst[i] = src0[i] * src1[-i];
}

void wma_window(WMA_Intern *p_WF, float *out)
{
	float *in = p_WF->output;
	int blocklen, bsize, n;

	// left part
	if (p_WF->blocklen_bits <= p_WF->prev_blocklen_bits)
	{
		blocklen = p_WF->blocklen;
		bsize = p_WF->mdctlen_bits - p_WF->blocklen_bits;

		vector_fmul_add_add(out, in, p_WF->windows[bsize], out, 0, blocklen, 1);
	}
	else
	{
		blocklen = 1 << p_WF->prev_blocklen_bits;
		n = (p_WF->blocklen - blocklen) / 2;
		bsize = p_WF->mdctlen_bits - p_WF->blocklen_bits;

		vector_fmul_add_add(out + n, in + n, p_WF->windows[bsize], out + n, 0,
			blocklen, 1);

		memcpy(out + n + blocklen, in + n + blocklen, n * sizeof(float));
	}

	out += p_WF->blocklen;
	in += p_WF->blocklen;

	// right part
	if (p_WF->blocklen_bits <= p_WF->next_blocklen_bits)
	{
		blocklen = p_WF->blocklen;
		bsize = p_WF->mdctlen_bits - p_WF->blocklen_bits;

		vector_fmul_reverse(out, in, p_WF->windows[bsize], blocklen);
	}
	else
	{
		blocklen = 1 << p_WF->next_blocklen_bits;
		n = (p_WF->blocklen - blocklen) / 2;
		bsize = p_WF->mdctlen_bits - p_WF->next_blocklen_bits;

		memcpy(out, in, n * sizeof(float));
		vector_fmul_reverse(out + n, in + n, p_WF->windows[bsize], blocklen);
		memset(out + n + blocklen, 0, n * sizeof(float));
	}
}

int DecodeBlock(WMA_Intern *p_WF)
{
	int i, nb_coefs[2], bsize;
	DWORD total_gain = 1, coef_nb_bits;
	static char mdctlen_bits = p_WF->mdctlen_bits;
	static char blocklen_bits = p_WF->blocklen_bits;
	static long mdctlen = p_WF->mdctlen;
	bool MS_Stereo = false, channel_coded[2] = {false, false};
	float mdct_norm;
	DWORD hb_coded[MAX_CHANNELS][HB_MAX_SIZE];

	if (feof(p_WF->f_WMA) != FALSE)
		return -2;
	if (pkt == NULL)
	{
		pkt = (BYTE *)malloc(8192);
		pktlen = fread(pkt, 1, 8192, p_WF->f_WMA);
	}
	else if (p_WF->p_Reader->nBytesRead >= pktlen)
		pktlen = fread(pkt, 1, 8192, p_WF->f_WMA);

	if ((p_WF->flags2 & 4) == 8) // Var block len
	{
		int n = log2(p_WF->nb_block_sizes - 1) + 1;
		int v;

		if (p_WF->reset_blocklengths != 0)
		{
			p_WF->reset_blocklengths = 0;
			v = ReadNBits(n, p_WF->p_Reader);
			if (v >= p_WF->nb_block_sizes)
				return -1;
			p_WF->prev_blocklen_bits = p_WF->mdctlen_bits - v;
			v = ReadNBits(n, p_WF->p_Reader);
			if (v >= p_WF->nb_block_sizes)
				return -1;
			p_WF->blocklen_bits = p_WF->mdctlen_bits - v;
		}
		else
		{
			p_WF->prev_blocklen_bits = p_WF->blocklen_bits;
			p_WF->blocklen_bits = p_WF->next_blocklen_bits;
		}
		v = ReadNBits(n, p_WF->p_Reader);
		if (v >= p_WF->nb_block_sizes)
			return -1;
		p_WF->next_blocklen_bits = p_WF->mdctlen_bits - v;
	}
	else
	{
		p_WF->next_blocklen_bits = p_WF->mdctlen_bits;
		p_WF->prev_blocklen_bits = p_WF->mdctlen_bits;
		p_WF->blocklen_bits = p_WF->mdctlen_bits;
	}

	if (p_WF->p_FI->Channels == 2)
		MS_Stereo = (bool)(ReadNBits(1, p_WF->p_Reader) & 0x01);

	{
		bool v = false;
		for (i = 0; i < p_WF->p_FI->Channels; i++)
		{
			channel_coded[i] = (bool)(ReadNBits(1, p_WF->p_Reader) & 0x01);
			v |= channel_coded[i];
		}

		if (v == false)
		{
			p_WF->nFrameLen = 0;
			goto next;
		}
	}

	{
		DWORD a;
		do
		{
			a = ReadNBits(7, p_WF->p_Reader);
			total_gain += a;
		}
		while (a == 127);
	}

	coef_nb_bits = total_gain_to_bits(total_gain);
	bsize = mdctlen_bits - blocklen_bits;

	{
		int n = p_WF->coefs_end[bsize] - p_WF->coefs_start;
		for (int i = 0; i < p_WF->p_FI->Channels; i++)
			nb_coefs[i] = n;
	}

	if (p_WF->noise_coding == true)
	{
		DWORD j;

		for (i = 0; i < p_WF->p_FI->Channels; i++)
		{
			if (channel_coded[i] == true)
			{
				for (j = 0; j < p_WF->exp_high_sizes[bsize]; j++)
				{
					int a = ReadNBits(1, p_WF->p_Reader);
					hb_coded[i][j] = a;
					if (a)
						nb_coefs[i] -= p_WF->exp_hbs[bsize][j];
				}
			}
		}
		for (i = 0; i < p_WF->p_FI->Channels; i++)
		{
			if (channel_coded[i] == true)
			{
				DWORD val, code;

				val = 0x80000000;
				for (j = 0; j < p_WF->exp_high_sizes[bsize]; j++)
				{
					if (val == 0x80000000)
						val = ReadNBits(7, p_WF->p_Reader) - 19;
					else
					{
						code = ReadVLC(p_WF->p_Reader, p_WF->hgain_vlc.table, 9, (13 + 9 - 1) / 9);
						if (code < 0)
							return -1;
						val += code - 18;
					}
					p_WF->hb_values[i][j] = val;
				}
			}
		}
	}

	if ((blocklen_bits == mdctlen_bits) || ReadNBits(1, p_WF->p_Reader) != 0)
	{
		for (i = 0; i < p_WF->p_FI->Channels; i++)
		{
			if (channel_coded[i] == true)
			{
				if (p_WF->use_exp_vlc == true)
				{
					if (ReadExpVLC(i, p_WF) < 0)
						return -1;
				}
				else
					ReadExpLSP(i, p_WF);
				p_WF->exps_bsize[i] = bsize;
			}
		}
	}

	for (i = 0; i < p_WF->p_FI->Channels; i++)
	{
		if (channel_coded[i] == true)
		{
			VLC *coef_vlc;
			int tbl_idx;
			int lvl, run, sign;
			const WORD *lvl_tbl, *run_tbl;
			short *ptr, *ptr_end;

			tbl_idx = (int)(i == 1 && MS_Stereo);
			coef_vlc = &p_WF->coef_vlc[tbl_idx];
			run_tbl = p_WF->run_tbl[tbl_idx];
			lvl_tbl = p_WF->lvl_tbl[tbl_idx];

			ptr = &p_WF->coefs1[i][0];
			ptr_end = ptr + nb_coefs[i];
			memset(ptr, 0x00, p_WF->blocklen * 2);

			while (ptr < ptr_end)
			{
				int code = ReadVLC(p_WF->p_Reader, coef_vlc->table, 9, 3);
				if (code < 0)
					return -1;
				else if (code == 1)
					break;
				else if (code == 0)
				{
					lvl = ReadNBits(coef_nb_bits, p_WF->p_Reader);
					run = ReadNBits(p_WF->blocklen_bits, p_WF->p_Reader);
				}
				else
				{
					run = run_tbl[code];
					lvl = lvl_tbl[code];
				}

				sign = ReadNBits(1, p_WF->p_Reader);
				if (sign == 0)
					lvl = -lvl;
				ptr += run;
				if (ptr >= ptr_end)
					break;
				*ptr = lvl;
				ptr++;
			}
		}

		if (p_WF->WMAVersion == 1 && p_WF->p_FI->Channels >= 2)
			AlignBits(p_WF->p_Reader);
	}

	{
		int n = p_WF->blocklen / 2;
		mdct_norm = 1.0F / (float)n;
		if (p_WF->WMAVersion == 1)
			mdct_norm *= sqrt((float)n);
	}

	for (i = 0; i < p_WF->p_FI->Channels; i++)
	{
		if (channel_coded[i] == true)
		{
			float *coefs, *exps, mult, exp_power[16], noise, mul;
			int size_exp, lhb = 0, j;
			short *coefs1;

			coefs1 = p_WF->coefs1[i];
			coefs = p_WF->coefs[i];
			exps = p_WF->exps[i];
			size_exp = p_WF->exps_bsize[i];
			mult = pow(10, total_gain * 0.05F) / p_WF->max_exp[i];
			mult *= mdct_norm;

			if (p_WF->noise_coding == true)
			{
				for (j = 0; j < p_WF->coefs_start; j++)
				{
					*coefs = p_WF->noise_tbl[p_WF->noise_idx] * exps[j << bsize >> size_exp] * mult;
					p_WF->noise_idx = (p_WF->noise_idx + 1) & 8191;
					coefs++;
				}

				exps = p_WF->exps[i] + (p_WF->hb_start[bsize] << bsize);

				for (j = 0; i < (int)p_WF->exp_high_sizes[bsize]; i++)
				{
					int n = p_WF->exp_hbs[bsize][j];
					if (hb_coded[i][j] > 0)
					{
						int k;
						float e = 0;

						for (k = 0; k < n; k++)
						{
							e += exps[i << bsize >> size_exp] * exps[i << bsize >> size_exp];
						}

						exp_power[j] = e / (float)n;
						lhb = j;
					}
					exps += n << bsize;
				}

				exps = p_WF->exps[i] + (p_WF->coefs_start << bsize);
				for (j = -1; j < (int)p_WF->exp_high_sizes[bsize]; j++)
				{
					int m, k;

					if (j < 0)
						m = p_WF->hb_start[bsize] - p_WF->coefs_start;
					else
						m = p_WF->exp_hbs[bsize][j];

					if (j >= 0 && hb_coded[i][j])
					{
						mul = sqrt(exp_power[j] / exp_power[lhb]);
						mul *= pow(10, p_WF->hb_values[i][j] / 20.0F);
						mul /= p_WF->max_exp[i] * p_WF->noise_mul;
						mul *= mdct_norm;

						for (k = 0; k < m; k++)
						{
							noise = p_WF->noise_tbl[p_WF->noise_idx];
							p_WF->noise_idx = (p_WF->noise_idx + 1) & 8191;
							*coefs = noise * exps[i << bsize >> size_exp] * mul;
							coefs++;
						}

						exps += m << bsize;
					}
					else
					{
						for (k = 0; k < m; k++)
						{
							noise = p_WF->noise_tbl[p_WF->noise_idx];
							p_WF->noise_idx = (p_WF->noise_idx + 1) & 8191;
							*coefs = noise * exps[i << bsize >> size_exp] * mul;
							coefs++;
						}
						exps += m << bsize;
					}
				}

				mul = mult * exps[(-1 << bsize) >> size_exp];
				for (j = 0; j < ((int)p_WF->blocklen - p_WF->coefs_end[bsize]); j++)
				{
					*coefs = p_WF->noise_tbl[p_WF->noise_idx] * mult;
					p_WF->noise_idx = (p_WF->noise_idx + 1) & 8191;
					coefs++;
				}
			}
			else
			{
				int n;
				for (j = 0; j < p_WF->coefs_start; j++)
				{
					*coefs = 0.0F;
					coefs++;
				}

				for (j = 0; j < nb_coefs[i]; j++)
				{
					*coefs = coefs1[j] * exps[i << bsize >> size_exp] * mult;
					coefs++;
				}

				n = p_WF->blocklen - p_WF->coefs_end[bsize];
				for (j = 0; j < n; j++)
				{
					*coefs = 0.0F;
					coefs++;
				}
			}
		}
	}

	if (MS_Stereo == true && channel_coded[1] == true)
	{
		if (channel_coded[0] == false)
		{
			memset(p_WF->coefs[0], 0x00, sizeof(float) * p_WF->blocklen);
			channel_coded[0] = true;
		}

		for (i = 0; i < (int)p_WF->blocklen; i++)
		{
			float a = p_WF->coefs[0][i];
			float b = p_WF->coefs[1][i];
			p_WF->coefs[0][i] = a + b;
			p_WF->coefs[1][i] = a - b;
		}
	}

	for (i = 0; i < p_WF->p_FI->Channels; i++)
	{
		if (channel_coded[i] == true)
		{
			int index, n = p_WF->blocklen / 2;

			imdct(&p_WF->imdct_ctx[bsize], p_WF->output, p_WF->coefs[i], (FFTComplex *)p_WF->mdcttmp);

			// perform windowing
			index = (p_WF->mdctlen / 2) + p_WF->blockpos - n;
			wma_window(p_WF, &p_WF->frame[i][index]);

			if (MS_Stereo == true && channel_coded[1] == false)
				wma_window(p_WF, &p_WF->frame[1][index]);
		}
	}

next:
	p_WF->blockpos += p_WF->blocklen;
	if (p_WF->p_Reader->nBytesRead >= pktlen)
	{
		return 1;
	}
	else
		return 0;
}

WORD clip_WORD(int a)
{
	if (((a + 32768) & ~65535) > 0)
		return (WORD)((a >> 31) ^ 32767);
	else
		return (WORD)(a);
}

int DecodeFrame(WMA_Intern *p_WF, short *samples)
{
	int ret = 0, n = p_WF->mdctlen, i;

	p_WF->blocknum = 0;
	p_WF->blockpos = 0;

	while (ret == 0)
	{
		ret = DecodeBlock(p_WF);
		if (ret < 0)
			return -1;
	}

	for (i = 0; i < p_WF->p_FI->Channels; i++)
	{
		short *ptr = samples + i;
		float *fptr = p_WF->frame[i];
		int j;

		for (j = 0; j < n; j++)
		{
			*ptr = clip_WORD((int)fabs(*fptr));
			fptr++;
			ptr += p_WF->p_FI->Channels;
		}
		memmove(p_WF->frame[i], p_WF->frame[i] + p_WF->mdctlen, p_WF->mdctlen * sizeof(float));
	}
	fwrite(samples, p_WF->p_FI->Channels * n, 1, p_WF->f_Out);

	return p_WF->blockpos;
}

int DecodeSuperFrame(WMA_Intern *p_WF, BYTE *OutBuffer)
{
	int nframes, bitoffset, i, len, pos;
	short *samples = (short *)OutBuffer;
	if (feof(p_WF->f_WMA) != FALSE)
		return -2;

	if (pkt == NULL)
	{
		pkt = (BYTE *)malloc(8192);
		pktlen = fread(pkt, 1, 8192, p_WF->f_WMA);
	}
	else if (p_WF->p_Reader->nBytesRead >= pktlen)
		pktlen = fread(pkt, 1, 8192, p_WF->f_WMA);
	init_bitstream_reader(p_WF, pkt, pktlen);

	// Uses a bit reservoir?
	if ((p_WF->flags2 & 2) != 0)
	{
		BYTE *q;

		// Skip the S-Frame index
		SkipNBits(4, p_WF->p_Reader);
		nframes = ReadNBits(4, p_WF->p_Reader) - 1;
		bitoffset = ReadNBits(p_WF->byteoffset_bits + 3, p_WF->p_Reader);

		if (p_WF->lastsuperframe_len > 0)
		{
			// add bitoffset bits to the last frame
			if (p_WF->lastsuperframe_len + ((bitoffset + 7) >> 3) > MAX_CODED_SUPERFRAME_SIZE)
				goto fail;
			q = p_WF->lastsuperframe + p_WF->lastsuperframe_len;
			len = bitoffset;
			while (len > 7)
			{
				*q = ReadNBits(8, p_WF->p_Reader);
				q++;
				len -= 8;
			}
			if (len > 0)
			{
				*q = ReadNBits(len, p_WF->p_Reader) << (8 - len);
				q++;
			}

			init_bitstream_reader(p_WF, p_WF->lastsuperframe, MAX_CODED_SUPERFRAME_SIZE);// * 8);
			if (p_WF->last_bitoffset > 0)
				SkipNBits(p_WF->last_bitoffset, p_WF->p_Reader);
			if (DecodeFrame(p_WF, samples) < 0)
				goto fail;
			samples += p_WF->p_FI->Channels * p_WF->mdctlen;
		}

		pos = bitoffset + 11 + p_WF->byteoffset_bits;
		init_bitstream_reader(p_WF, pkt + (pos >> 3), (MAX_CODED_SUPERFRAME_SIZE - (pos >> 3)));// * 8);
		len = pos & 7;
		if (len > 0)
			SkipNBits(len, p_WF->p_Reader);

		p_WF->reset_blocklengths = 1;
		for (i = 0; i < nframes; i++)
		{
			if (DecodeFrame(p_WF, samples) < 0)
				goto fail;
			samples += p_WF->p_FI->Channels * p_WF->mdctlen;
		}

		pos = (p_WF->p_Reader->nBitsRead + ((p_WF->p_Reader->nBytesRead - 1) * 8)) + ((bitoffset + 11 + p_WF->byteoffset_bits) & ~7);
		p_WF->last_bitoffset = pos & 7;
		pos >>= 3;
		len = pktlen - pos;
		if (len > MAX_CODED_SUPERFRAME_SIZE || len < 0)
			goto fail;
		p_WF->lastsuperframe_len = len;
		memcpy(p_WF->lastsuperframe, pkt + pos, len);
	}
	else
	{
		if (DecodeFrame(p_WF, samples) < 0)
			goto fail;
		samples += p_WF->p_FI->Channels * p_WF->mdctlen;
	}

	return ((BYTE *)samples) - OutBuffer;
fail:
	p_WF->lastsuperframe_len = 0;
	return -1;
}

int main(int argc, char **argv)
{
	WMA_Intern *p_WF;
	FileInfo *p_FI;
	BYTE buffer[16384];
	int ret = 1;
	FILE *f_Meta = fopen("Breezin.wma.meta", "rb");
	FILE *f_WMA = fopen("Breezin.wma.audio", "rb");
	p_WF = (WMA_Intern *)malloc(sizeof(WMA_Intern));
	memset(p_WF, 0x00, sizeof(WMA_Intern));
	p_WF->p_ASFStrm.push_back((ASFStreamHeader *)malloc(sizeof(ASFStreamHeader)));
	memset(p_WF->p_ASFStrm[0], 0x00, sizeof(ASFStreamHeader));
	fread(p_WF->p_ASFStrm[0], sizeof(ASFStreamHeader) - (2 * sizeof(BYTE)), 1, f_Meta);
	p_WF->p_ASFStrm[0]->data = (BYTE *)malloc(p_WF->p_ASFStrm[0]->data_len);
	fread(p_WF->p_ASFStrm[0]->data, p_WF->p_ASFStrm[0]->data_len, 1, f_Meta);
	p_WF->p_ASFStrm[0]->errcorr = (BYTE *)malloc(p_WF->p_ASFStrm[0]->errcorr_len);
	fread(p_WF->p_ASFStrm[0]->errcorr, p_WF->p_ASFStrm[0]->errcorr_len, 1, f_Meta);
	fclose(f_Meta);
	p_FI = WMA_GetFileInfo(p_WF);
	p_WF->f_WMA = f_WMA;
	p_WF->f_Out = fopen("O:\\Breezin.raw", "wb");

	//DecodeFrame(p_WF, (short *)buffer);
	while (ret >= 0)
		ret = DecodeSuperFrame(p_WF, buffer);

	fclose(p_WF->f_Out);
	fclose(f_WMA);
	return 0;
}
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <malloc.h>
#ifdef _WINDOWS
#include <windows.h>
#endif
#include <math.h>
#include <string.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "ImpulseTracker.h"

#define VOLCMD_VOLUME		1
#define VOLCMD_PANNING		2
#define VOLCMD_VOLSLIDEUP	3
#define VOLCMD_VOLSLIDEDOWN	4
#define VOLCMD_FINEVOLUP	5
#define VOLCMD_FINEVOLDOWN	6
#define VOLCMD_PITCHUP		7
#define VOLCMD_PITCHDOWN	8
#define VOLCMD_PORTATO		9
#define VOLCMD_VIBRATO		10

#define CMD_SPEED			65
#define CMD_POSITIONJUMP	66
#define CMD_PATTERNBREAK	67
#define CMD_VOLUMESLIDE		68
#define CMD_PORTAMENTODOWN	69
#define CMD_PORTAMENTOUP	70
#define CMD_TONEPORTAMENTO	71
#define CMD_VIBRATO			72
#define CMD_TREMOR			73
#define CMD_ARPEGGIO		74
#define CMD_VIBRATOVOL		75
#define CMD_TONEPORTAVOL	76
#define CMD_CHANNELVOLUME	77
#define CMD_CHANNELVOLSLIDE	78
#define CMD_OFFSET			79
#define CMD_PANNINGSLIDE	80
#define CMD_RETRIG			81
#define CMD_TREMOLO			82
#define CMD_S3MCMDEX		83
#define CMD_TEMPO			84
#define CMD_FINEVIBRATO		85
#define CMD_GLOBALVOLUME	86
#define CMD_GLOBALVOLSLIDE	87
#define CMD_PANNING8		88
#define CMD_PANBRELLO		89
#define CMD_MIDI			90

void *IT_OpenR(const char *FileName)
{
	IT_Intern *ret = NULL;
	FILE *f_IT = NULL;

	ret = (IT_Intern *)malloc(sizeof(IT_Intern));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(IT_Intern));

	f_IT = fopen(FileName, "rb");
	if (f_IT == NULL)
		return f_IT;

	ret->f_IT = f_IT;
	ret->p_Head = (ITFileHeader *)malloc(sizeof(ITFileHeader));

	return ret;
}

void PrintVolCmd(BYTE vol, BYTE cmd, FILE *f_Out)
{
	switch (cmd)
	{
		case VOLCMD_VOLUME:
			fprintf(f_Out, "v%02i ", vol);
			return;
		case VOLCMD_PANNING:
			fprintf(f_Out, "p%02i ", vol);
			return;
		case VOLCMD_VOLSLIDEUP:
			fprintf(f_Out, "c%02i ", vol);
			return;
		case VOLCMD_VOLSLIDEDOWN:
			fprintf(f_Out, "d%02i ", vol);
			return;
		case VOLCMD_FINEVOLUP:
			fprintf(f_Out, "a%02i ", vol);
			return;
		case VOLCMD_FINEVOLDOWN:
			fprintf(f_Out, "b%02i ", vol);
			return;
		case VOLCMD_PITCHUP:
			fprintf(f_Out, "f%02i ", vol);
			return;
		case VOLCMD_PITCHDOWN:
			fprintf(f_Out, "e%02i ", vol);
			return;
		case VOLCMD_PORTATO:
			fprintf(f_Out, "g%02i ", vol);
			return;
		case VOLCMD_VIBRATO:
			fprintf(f_Out, "u%02i ", vol);
			return;
	}
}

void PrintPaterns(Patern *p_Paterns, UINT nPaterns, FileInfo *p_FI)
{
	FILE *f_Out = fopen("Out.log", "wb");
	for (UINT i = 0; i < nPaterns; i++)
	{
		Command *Commands = p_Paterns[i].p_Commands;
		fprintf(f_Out, "Patern #%i: \n", i);
		for (USHORT j = 0; j < p_Paterns[i].nRows; j++)
		{
			fprintf(f_Out, "%02X:\t", j);
			for (int k = 0; k < p_FI->Channels; k++)
			{
				Command cmd = Commands[(j * p_FI->Channels) + k];
				if (cmd.note == 0)
					fprintf(f_Out, ".. ");
				else if (cmd.note < 120)
					fprintf(f_Out, "%02X ", cmd.note);
				else if (cmd.note == 255)
					fprintf(f_Out, "== ", cmd.note);
				else //if (cmd.note == 254)
					fprintf(f_Out, "^^ ", cmd.note);

				if (cmd.instr == 0)
					fprintf(f_Out, ".. ");
				else
					fprintf(f_Out, "%02i ", cmd.instr);

				if (cmd.volcmd == 0)
					fprintf(f_Out, "... ");
				else
					PrintVolCmd(cmd.vol, cmd.volcmd, f_Out);

				if (cmd.command == 0)
					fprintf(f_Out, "...");
				else
					fprintf(f_Out, "%c%02X", cmd.command, cmd.param);

				if ((k + 1) == p_FI->Channels)
					fprintf(f_Out, "\n");
				else
					fprintf(f_Out, "\t");
			}
		}
	}
}

void IT_ReadSample(ITSampleStruct *smp, SampleData *data, FILE *f_IT)
{
	if ((smp->flags & 0x08) == 0) // Not a compressed sample?
	{
		smp->length *= (data->BPS / 8);
		data->length = smp->length;// * (data->BPS / 8);
		data->PCM = (BYTE *)malloc(data->length * 2);
		memset(data->PCM, 0x00, data->length * 2);
		fread(data->PCM, data->length, 1, f_IT);
		if ((smp->cvt & 0x01) == 0) // Unsigned?
		{
			UINT i;
			if (data->BPS == 8)
			{
				for (i = 0; i < smp->length; i++)
					data->PCM[i] = (BYTE)(((short)(data->PCM[i])) - 0x80);
			}
			else
			{
				for (i = 0; i < smp->length; i++)
				{
					short val = ((short *)data->PCM)[i];
					((short *)data->PCM)[i] = (short)(((int)val) - 0x8000);
				}
			}
		}
	}
	else // Compressed in some way
	{
	}
}

#ifdef _WINDOWS
#define snprintf _snprintf
#endif

FileInfo *IT_GetFileInfo(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	FileInfo *ret = NULL;
	FILE *f_IT = p_IF->f_IT;
	char ID[4];

	ret = (FileInfo *)malloc(sizeof(FileInfo));
	if (ret == NULL)
		return ret;
	memset(ret, 0x00, sizeof(FileInfo));

	fread(p_IF->p_Head, sizeof(ITFileHeader), 1, f_IT);
	if (strncmp((const char *)p_IF->p_Head->id, "IMPM", 4) != 0 ||
		p_IF->p_Head->reserved1 != 0x1004)
		return NULL;

	ret->Title = (char *)p_IF->p_Head->songname;
	ret->BitRate = 44100;
	ret->BitsPerSample = 16;
	ret->Channels = 2;
	p_IF->p_FI = ret;

	if (p_IF->p_Head->msgoffset != 0)
	{
		char *msg = (char *)malloc(p_IF->p_Head->msglength + 1);
		fseek(f_IT, p_IF->p_Head->msgoffset, SEEK_SET);
		fread(msg, p_IF->p_Head->msglength, 1, f_IT);
		msg[p_IF->p_Head->msglength] = 0;
		ret->OtherComments.push_back(msg);
		ret->nOtherComments = 1;
		msg = NULL;
	}

	if (p_IF->p_Head->ordnum != 0)
	{
		fseek(f_IT, sizeof(ITFileHeader), SEEK_SET);
		printf("IT File has %i sections\n", p_IF->p_Head->ordnum);
		p_IF->p_PaternOrder = (BYTE *)malloc(p_IF->p_Head->ordnum);
		fread(p_IF->p_PaternOrder, p_IF->p_Head->ordnum, 1, f_IT);
	}

	if (p_IF->p_Head->insnum != 0)
	{
		printf("IT File has %i instruments\n", p_IF->p_Head->insnum);
		p_IF->p_InstOffsets = (UINT *)malloc(sizeof(UINT) * p_IF->p_Head->insnum);
		fread(p_IF->p_InstOffsets, 4, p_IF->p_Head->insnum, f_IT);
	}

	if (p_IF->p_Head->smpnum != 0)
	{
		printf("IT File has %i samples\n", p_IF->p_Head->smpnum);
		p_IF->p_SampOffsets = (UINT *)malloc(sizeof(UINT) * p_IF->p_Head->smpnum);
		fread(p_IF->p_SampOffsets, 4, p_IF->p_Head->smpnum, f_IT);
	}

	if (p_IF->p_Head->patnum != 0)
	{
		printf("IT File has %i paterns\n", p_IF->p_Head->patnum);
		p_IF->p_PaternOffsets = (UINT *)malloc(sizeof(UINT) *p_IF->p_Head->patnum);
		fread(p_IF->p_PaternOffsets, 4, p_IF->p_Head->patnum, f_IT);
	}

	{
		// Discardable extra:
		USHORT len;
		char *Unknown;
		fread(&len, 2, 1, f_IT);
		Unknown = (char *)malloc(len);
		fread(Unknown, len, 1, f_IT);
		free(Unknown);
	}

	if ((p_IF->p_Head->flags & 0x80) != 0)
	{
		p_IF->p_MidiCfg = (MidiConfig *)malloc(sizeof(MidiConfig));
		fread(&p_IF->p_MidiCfg, sizeof(MidiConfig), 1, f_IT);
	}

	fread(ID, 4, 1, f_IT);

	if (ID == "PNAM")
	{
		fread(&p_IF->PatName.nNames, 4, 1, f_IT);
		p_IF->PatName.nNames /= 20;
		p_IF->PatName.p_Names = (BYTE **)malloc(sizeof(BYTE *) * p_IF->PatName.nNames);
		for (UINT i = 0; i < p_IF->PatName.nNames; i++)
		{
			p_IF->PatName.p_Names[i] = (BYTE *)malloc(20);
			fread(p_IF->PatName.p_Names[i], 20, 1, f_IT);
		}
	}
	else
		fseek(f_IT, -4, SEEK_CUR);

	fread(ID, 4, 1, f_IT);

	if (ID == "CNAME")
	{
		fread(&p_IF->ChanName.nNames, 4, 1, f_IT);
		p_IF->ChanName.nNames /= 20;
		p_IF->ChanName.p_Names = (BYTE **)malloc(sizeof(BYTE *) * p_IF->ChanName.nNames);
		for (UINT i = 0; i < p_IF->ChanName.nNames; i++)
		{
			p_IF->ChanName.p_Names[i] = (BYTE *)malloc(20);
			fread(p_IF->ChanName.p_Names[i], 20, 1, f_IT);
		}
	}
	else
		fseek(f_IT, -4, SEEK_CUR);

	if (p_IF->p_Head->insnum != 0)
	{
		printf("Reading intstruments.....\n\n");

		// Allocate enough memory
		if (p_IF->p_Head->cmwt < 0x200)
			p_IF->p_OldIns = (ITOldInstrument *)malloc(sizeof(ITOldInstrument) * p_IF->p_Head->insnum);
		else
			p_IF->p_Ins = (ITInstrument *)malloc(sizeof(ITInstrument) * p_IF->p_Head->insnum);

		// Read in the instruments
		for (WORD i = 0; i < p_IF->p_Head->insnum; i++)
		{
			fseek(f_IT, p_IF->p_InstOffsets[i], SEEK_SET);
			if (p_IF->p_Head->cmwt < 0x200)
				fread(&p_IF->p_OldIns[i], sizeof(ITOldInstrument), 1, f_IT);
			else
				fread(&p_IF->p_Ins[i], sizeof(ITInstrument), 1, f_IT);
		}
	}

	if (p_IF->p_Head->smpnum != 0)
	{
		printf("Reading samples.....\n\n");
		p_IF->p_Samp = (ITSampleStruct *)malloc(sizeof(ITSampleStruct) * p_IF->p_Head->smpnum);
		p_IF->p_Samples = (SampleData *)malloc(sizeof(SampleData) * p_IF->p_Head->smpnum);

		for (WORD i = 0; i < p_IF->p_Head->smpnum; i++)
		{
			// Get the sample header
			memset(&p_IF->p_Samp[i], 0x00, sizeof(ITSampleStruct));
			fseek(f_IT, p_IF->p_SampOffsets[i], SEEK_SET);
			fread(&p_IF->p_Samp[i], sizeof(ITSampleStruct), 1, f_IT);
			if (p_IF->p_Samp[i].C5Speed == 0)
				p_IF->p_Samp[i].C5Speed = 8363;
			else if (p_IF->p_Samp[i].C5Speed < 256)
				p_IF->p_Samp[i].C5Speed = 256;
			else
				p_IF->p_Samp[i].C5Speed /= 2;

			// And now the data
			fseek(f_IT, p_IF->p_Samp[i].samplepointer, SEEK_SET);
			p_IF->p_Samples[i].BPS = ((p_IF->p_Samp[i].flags & 0x02) != 0 ? 16 : 8);
			IT_ReadSample(&p_IF->p_Samp[i], &p_IF->p_Samples[i], f_IT);

			printf("%02i: ", i);
			printf("%s\n", p_IF->p_Samp[i].name);
		}
		printf("\n");
	}

	// This code works flawlessly on a properly encoded/saved
	// .IT file. MPT Compatability files from the current release
	// do not follow the standard properly, so this messes up.
	for (int i = 0; i < 64; i++)
	{
		if (p_IF->p_Head->chnpan[i] > 128)
		{
			p_IF->nChannels = i;
			break;
		}
	}
	if (p_IF->nChannels == 0)
		p_IF->nChannels = 64;
	if (p_IF->nChannels < 4)
		p_IF->nChannels = 4;

	if (p_IF->p_Head->patnum != 0)
	{
		printf("Reading paterns.....\n\n");
		p_IF->p_Paterns = (Patern *)malloc(sizeof(Patern) * p_IF->p_Head->patnum);
		memset(p_IF->p_Paterns, 0x00, sizeof(Patern) * p_IF->p_Head->patnum);

		for (UINT i = 0; i < p_IF->p_Head->patnum; i++)
		{
			BYTE b = 0, ChanMask[64];
			WORD Rows, Len;
			WORD nRows = 0, j = 0;
			Command LastVal[64];
			Command *Commands = NULL;
			if (p_IF->p_PaternOffsets[i] == 0)
				continue;

			fseek(f_IT, p_IF->p_PaternOffsets[i], SEEK_SET);
			fread(&Len, 2, 1, f_IT);
			p_IF->p_Paterns[i].PatLen = Len;
			fread(&Rows, 2, 1, f_IT);
			p_IF->p_Paterns[i].nRows = Rows;
			fseek(f_IT, 4, SEEK_CUR);

			p_IF->p_Paterns[i].p_Commands = (Command *)malloc(sizeof(Command) * p_IF->nChannels * p_IF->p_Paterns[i].nRows);
			memset(ChanMask, 0x00, sizeof(ChanMask));
			memset(LastVal, 0x00, sizeof(LastVal));
			memset(p_IF->p_Paterns[i].p_Commands, 0x00, sizeof(Command) * p_IF->nChannels * p_IF->p_Paterns[i].nRows);
			Commands = p_IF->p_Paterns[i].p_Commands;

			while (nRows < Rows)
			{
				int nChan = 0;
				UINT nPatPos;
				if (j > Len)
					break;

				fread(&b, 1, 1, f_IT);
				j++;
				// Check for end of row
				if (b == 0)
				{
					nRows++;
					continue;
				}
				// Get channel number
				nChan = b & 0x7F;
				if (nChan != 0)
					nChan = (nChan - 1) & 0x3F;
				// Set up the Position number
				nPatPos = (nRows * p_IF->nChannels) + nChan;
				// Check for ChanMask
				if ((b & 0x80) != 0)
				{
					if (j >= Len)
						break;
					fread(&ChanMask[nChan], 1, 1, f_IT);
					j++;
				}
				// Check for a repeated note
				if ((ChanMask[nChan] & 0x10) != 0 && nChan < p_IF->nChannels)
					Commands[nPatPos].note = LastVal[nChan].note;
				// Check for a repeated instrument
				if ((ChanMask[nChan] & 0x20) != 0 && nChan < p_IF->nChannels)
					Commands[nPatPos].instr = LastVal[nChan].instr;
				// Check for repeated volume command
				if ((ChanMask[nChan] & 0x40) != 0 && nChan < p_IF->nChannels)
				{
					Commands[nPatPos].vol = LastVal[nChan].vol;
					Commands[nPatPos].volcmd = LastVal[nChan].volcmd;
				}
				// Check for repeated parameter command
				if ((ChanMask[nChan] & 0x80) != 0 && nChan < p_IF->nChannels)
				{
					Commands[nPatPos].command = LastVal[nChan].command;
					Commands[nPatPos].param = LastVal[nChan].param;
				}
				// Check for a normal note
				if ((ChanMask[nChan] & 1) != 0)
				{
					BYTE note = 0;
					if (j >= Len)
						break;
					fread(&note, 1, 1, f_IT);
					j++;

					if (nChan < p_IF->nChannels)
					{
						if (note < 0x80)
							note++;
						Commands[nPatPos].note = note;
						LastVal[nChan].note = note;
					}
				}
				// Check for a normal instrument
				if ((ChanMask[nChan] & 2) != 0)
				{
					BYTE instr = 0;
					if (j >= Len)
						break;
					fread(&instr, 1, 1, f_IT);
					j++;

					if (nChan < p_IF->nChannels)
					{
						Commands[nPatPos].instr = instr;
						LastVal[nChan].instr = instr;
					}
				}
				// Check for a normal volume command
				if ((ChanMask[nChan] & 4) != 0)
				{
					BYTE vol = 0;
					if (j >= Len)
						break;
					fread(&vol, 1, 1, f_IT);
					j++;

					if (nChan < p_IF->nChannels)
					{
						if (vol <= 64)
						{
							Commands[nPatPos].vol = vol;
							Commands[nPatPos].volcmd = VOLCMD_VOLUME;
						}
						else if (vol <= 74)
						{
							Commands[nPatPos].vol = vol - 65;
							Commands[nPatPos].volcmd = VOLCMD_FINEVOLUP;
						}
						else if (vol <= 84)
						{
							Commands[nPatPos].vol = vol - 75;
							Commands[nPatPos].volcmd = VOLCMD_FINEVOLDOWN;
						}
						else if (vol <= 94)
						{
							Commands[nPatPos].vol = vol - 85;
							Commands[nPatPos].volcmd = VOLCMD_VOLSLIDEUP;
						}
						else if (vol <= 104)
						{
							Commands[nPatPos].vol = vol - 95;
							Commands[nPatPos].volcmd = VOLCMD_VOLSLIDEDOWN;
						}
						else if (vol <= 114)
						{
							Commands[nPatPos].vol = vol - 105;
							Commands[nPatPos].volcmd = VOLCMD_PITCHDOWN;
						}
						else if (vol <= 124)
						{
							Commands[nPatPos].vol = vol - 115;
							Commands[nPatPos].volcmd = VOLCMD_PITCHUP;
						}
						else if (vol >= 128 && vol <= 192)
						{
							Commands[nPatPos].vol = vol - 128;
							Commands[nPatPos].volcmd = VOLCMD_PANNING;
						}
						else if (vol >= 193 && vol <= 202)
						{
							Commands[nPatPos].vol = vol - 193;
							Commands[nPatPos].volcmd = VOLCMD_PORTATO;
						}
						else if (vol >= 203 && vol <= 212)
						{
							Commands[nPatPos].vol = vol - 203;
							Commands[nPatPos].volcmd = VOLCMD_VIBRATO;
						}
					}
				}
				// Check for a normal parameter command
				if ((ChanMask[nChan] & 8) != 0)
				{
					BYTE cmd = 0, param = 0;
					fread(&cmd, 1, 1, f_IT);
					fread(&param, 1, 1, f_IT);
					j += 2;

					if (cmd + 0x40 < 65 || cmd + 0x40 > 90)
						cmd = 0;
					else
						cmd += 0x40;
				}
			}
		}
	}

	if (ExternalPlayback == 0)
		p_IF->p_Playback = new Playback(ret, IT_FillBuffer, p_IF->buffer, 8192, p_ITFile);
	p_IF->p_SndFile = new ISoundFile(p_IF);
	//PrintPaterns(p_IF->p_Paterns, p_IF->p_Head->patnum, ret);

	return ret;
}

int IT_CloseFileR(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;

	delete p_IF->p_Playback;
	delete p_IF->p_SndFile;

	return fclose(p_IF->f_IT);
}

long IT_FillBuffer(void *p_ITFile, BYTE *OutBuffer, int OutBufferLen)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	DWORD Read = FillITBuffer(p_IF);
	if (Read != 0)
		memcpy(OutBuffer, p_IF->buffer, 8192);
	return Read;
}

void IT_Play(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;

	p_IF->p_Playback->Play();
}

bool Is_IT(const char *FileName)
{
	FILE *f_IT = fopen(FileName, "rb");
	CHAR ITSig[4];
	if (f_IT == NULL)
		return false;

	fread(ITSig, 4, 1, f_IT);
	fclose(f_IT);

	if (strncmp((const char *)ITSig, "IMPM", 4) != 0)
		return false;

	return true;
}

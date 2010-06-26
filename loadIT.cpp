#define _USE_MATH_DEFINES
#include <stdio.h>
#include <malloc.h>
#include <windows.h>
#include <math.h>

#include <al.h>
#include <alc.h>

#include "libAudio.h"
#include "libAudio_Common.h"
#include "ImpulseTracker.h"

/*static const double FrequencyMult[255] = 
{
	1, 1, 1,
	(16.35 / 523.25), (17.32 / 523.25),
	(18.35 / 523.25), (19.45 / 523.25),
	(20.60 / 523.25), (21.83 / 523.25),
	(23.12 / 523.25), (24.50 / 523.25),
	(25.96 / 523.25), (27.50 / 523.25),
	(29.14 / 523.25), (30.87 / 523.25),
	(32.70 / 523.25), (34.65 / 523.25),
	(36.71 / 523.25), (38.89 / 523.25),
	(41.20 / 523.25), (43.65 / 523.25),
	(46.25 / 523.25), (49.00 / 523.25),
	(51.91 / 523.25), (55.00 / 523.25),
	(58.27 / 523.25), (61.74 / 523.25),
	(65.41 / 523.25), (69.30 / 523.25),
	(73.42 / 523.25), (77.78 / 523.25),
	(82.41 / 523.25), (87.31 / 523.25),
	(92.50 / 523.25), (98.00 / 523.25),
	(103.83 / 523.25), (110.00 / 523.25),
	(116.54 / 523.25), (123.47 / 523.25),
	(130.81 / 523.25), (138.59 / 523.25),
	(146.83 / 523.25), (174.61 / 523.25),
	(185.00 / 523.25), (196.00 / 523.25),
	(207.65 / 523.25), (220.00 / 523.25),
	(233.08 / 523.25), (246.94 / 523.25),
	(261.63 / 523.25), (277.18 / 523.25),
	(293.66 / 523.25), (311.13 / 523.25),
	(329.63 / 523.25), (349.23 / 523.25),
	(369.99 / 523.25), (392.00 / 523.25),
	(415.30 / 523.25), (440.00 / 523.25),
	(466.16 / 523.25), (493.88 / 523.25),
	1.0,
	(554.37 / 523.25), (587.33 / 523.25),
	(622.25 / 523.25), (659.26 / 523.25),
	(698.46 / 523.25), (739.99 / 523.25),
	(783.99 / 523.25), (830.61 / 523.25),
	(880.00 / 523.25), (932.33 / 523.25),
	(987.77 / 523.25), (1046.50 / 523.25),
	(1108.73 / 523.25), (1174.66 / 523.25),
	(1244.51 / 523.25), (1318.51 / 523.25),
	(1396.91 / 523.25), (1479.98 / 523.25),
	(1567.98 / 523.25), (1661.22 / 523.25),
	(1760.00 / 523.25), (1864.66 / 523.25),
	(1975.53 / 523.25), (2093.00 / 523.25),
	(2217.46 / 523.25), (2349.32 / 523.25),
	(2489.02 / 523.25), (2637.02 / 523.25),
	(2793.83 / 523.25), (2959.96 / 523.25),
	(3135.96 / 523.25),	(3322.44 / 523.25),
	(3520.00 / 523.25), (3729.31 / 523.25),
	(3951.07 / 523.25), (4186.01 / 523.25),
	(4434.92 / 523.25), (4698.64 / 523.25),
	(4978.03 / 523.25)
};*/

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

void *IT_OpenR(char *FileName)
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
	if (strncmp(p_IF->p_Head->id, "IMPM", 4) != 0 ||
		p_IF->p_Head->reserved1 != 0x1004)
		return NULL;

	ret->Title = p_IF->p_Head->songname;
	ret->BitRate = 44100;
	ret->BitsPerSample = 16;
	for (int i = 0; i < 64; i++)
	{
		if (p_IF->p_Head->chnpan[i] > 128)
		{
			ret->Channels = i;
			break;
		}
	}
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

			// And now the data
			fseek(f_IT, p_IF->p_Samp[i].samplepointer, SEEK_SET);
			p_IF->p_Samples[i].BPS = ((p_IF->p_Samp[i].flags & 0x02) != 0 ? 16 : 8);
			p_IF->p_Samples[i].BitRate = p_IF->p_Samp[i].C5Speed;
			p_IF->p_Samples[i].length = p_IF->p_Samp[i].length * (p_IF->p_Samples[i].BPS / 8);
			p_IF->p_Samples[i].PCM = (BYTE *)malloc(p_IF->p_Samples[i].length);
			if ((p_IF->p_Samp[i].flags & 0x08) == 0)
				fread(p_IF->p_Samples[i].PCM, p_IF->p_Samples[i].length, 1, f_IT);

			printf("%02i: ", i);
			printf("%s\n", p_IF->p_Samp[i].name);
		}
		printf("\n");
	}

	//LoadMixPlugins(f_IT);

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
			fread(&p_IF->p_Paterns[i].PatLen, 2, 1, f_IT);
			fread(&p_IF->p_Paterns[i].nRows, 2, 1, f_IT);
			Rows = p_IF->p_Paterns[i].nRows;
			Len = p_IF->p_Paterns[i].PatLen;
			fseek(f_IT, 4, SEEK_CUR);

			p_IF->p_Paterns[i].p_Commands = (Command *)malloc(sizeof(Command) * ret->Channels * p_IF->p_Paterns[i].nRows);
			memset(ChanMask, 0x00, sizeof(ChanMask));
			memset(LastVal, 0x00, sizeof(LastVal));
			memset(p_IF->p_Paterns[i].p_Commands, 0x00, sizeof(Command) * ret->Channels * p_IF->p_Paterns[i].nRows);
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
				nPatPos = (nRows * ret->Channels) + nChan;
				// Check for ChanMask
				if ((b & 0x80) != 0)
				{
					if (j >= Len)
						break;
					fread(&ChanMask[nChan], 1, 1, f_IT);
					j++;
				}
				// Check for a repeated note
				if ((ChanMask[nChan] & 0x10) != 0 && nChan < ret->Channels)
				{
					Commands[nPatPos].note = LastVal[nChan].note;
				}
				// Check for a repeated instrument
				if ((ChanMask[nChan] & 0x20) != 0 && nChan < ret->Channels)
				{
					Commands[nPatPos].instr = LastVal[nChan].instr;
				}
				// Check for repeated volume command
				if ((ChanMask[nChan] & 0x40) != 0 && nChan < ret->Channels)
				{
					Commands[nPatPos].vol = LastVal[nChan].vol;
					Commands[nPatPos].volcmd = LastVal[nChan].volcmd;
				}
				// Check for repeated parameter command
				if ((ChanMask[nChan] & 0x80) != 0 && nChan < ret->Channels)
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

					if (nChan < ret->Channels)
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

					if (nChan < ret->Channels)
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

					if (nChan < ret->Channels)
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
	p_IF->p_Source = new Source(p_IF->p_SndFile);
	//PrintPaterns(p_IF->p_Paterns, p_IF->p_Head->patnum, ret);

	return ret;
}

int IT_CloseFileR(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;

	delete p_IF->p_Playback;
	delete p_IF->p_SndFile;
	delete p_IF->p_Source;

	return fclose(p_IF->f_IT);
}

long IT_FillBuffer(void *p_ITFile, BYTE *OutBuffer, int OutBufferLen)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;
	DWORD Read;

	Read = FillITBuffer(p_IF);
	return Read;
}

void IT_Play(void *p_ITFile)
{
	IT_Intern *p_IF = (IT_Intern *)p_ITFile;

	p_IF->p_Playback->Play();
}

bool Is_IT(char *FileName)
{
	FILE *f_IT = fopen(FileName, "rb");
	CHAR ITSig[4];
	if (f_IT == NULL)
		return false;

	fread(ITSig, 4, 1, f_IT);
	fclose(f_IT);

	if (strncmp(ITSig, "IMPM", 4) != 0)
		return false;

	return true;
}
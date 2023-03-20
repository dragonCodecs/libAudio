// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2012-2023 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <libAudio.h>
#include <string>
#include <substrate/fd>
// XXX: This header actually needs installing and the current header mess figured out + fixed.
#include "../libAudio/console.hxx"

using libAudio::console::asTime_t;

#if STREAMING
inline std::string operator""_s(const char *const str, const size_t len)
	{ return std::string{str, len}; }
#endif

#ifdef _WINDOWS
void invalidHandler(const wchar_t *, const wchar_t *, const wchar_t *, const uint32_t, const uintptr_t) { }
#endif

#if STREAMING
static void writeNowPlaying(const char *const file, const char *const title)
{
	using namespace substrate;
	fd_t fd{"/tmp/musicStreamer.playing", O_RDWR | O_NOCTTY | O_CREAT | O_TRUNC, normalMode};
	if (!fd.valid())
		return;
	fd.write("Now Playing: "_s);
	fd.write(file, strlen(file));
	fd.write(" ("_s);
	fd.write(title, strlen(title));
	fd.write(")    "_s);
}
#endif

int main(int argc, char **argv)
{
#if _WINDOWS
	_set_invalid_parameter_handler(invalidHandler);
	_CrtSetReportMode(_CRT_ASSERT, 0);
	_CrtSetReportMode(_CRT_ERROR, 0);
#endif
	if (argc < 2)
		return -1;
	console = {stdout, stderr};

	for (int i = 1; i < argc; i++)
	{
		void *audioFile = audioOpenR(argv[i]);
		if (!audioFile)
			continue;
		const auto info = audioGetFileInfo(audioFile);
		if (!info)
		{
			audioCloseFile(audioFile);
			continue;
		}

		console.info("File '", argv[i], "', TotalTime: ", asTime_t{info->totalTime}, ", Sample Rate: ", info->bitRate,
			"Hz, Title: ", info->title, ", Artist: ", info->artist, ", Album: ", info->album, ", Channels: ",
			info->channels);
#if STREAMING
		writeNowPlaying(argv[i], info->title.get());
#endif

		audioPlay(audioFile);
		audioCloseFile(audioFile);
	}
	return 0;
}

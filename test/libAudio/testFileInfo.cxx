// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Rachel Mant <git@dragonmux.network>
#include <string_view>
#include <crunch++.h>
#include <libAudio.h>
#include <fileInfo.hxx>
#include <string.hxx>

using namespace std::literals::string_view_literals;

constexpr static auto title{"title"sv};
constexpr static auto artist{"artist"sv};
constexpr static auto album{"album"sv};

class testFileInfo final : public testsuite
{
private:
	void testFileInfoCXX()
	{
		fileInfo_t fileInfo{};
		assertEqual(fileInfo.totalTime(), 0U);
		assertEqual(fileInfo.bitsPerSample(), 0U);
		assertEqual(fileInfo.bitRate(), 0U);
		assertEqual(fileInfo.channels(), 0U);
		assertNull(fileInfo.title());
		assertNull(fileInfo.artist());
		assertNull(fileInfo.album());

		fileInfo.totalTime(UINT64_MAX);
		fileInfo.bitsPerSample(UINT32_MAX);
		fileInfo.bitRate(UINT32_MAX);
		fileInfo.channels(UINT8_MAX);
		fileInfo.title(stringDup(title.data()));
		fileInfo.artist(stringDup(artist.data()));
		fileInfo.album(stringDup(album.data()));

		assertEqual(fileInfo.totalTime(), UINT64_MAX);
		assertEqual(fileInfo.bitsPerSample(), UINT32_MAX);
		assertEqual(fileInfo.bitRate(), UINT32_MAX);
		assertEqual(fileInfo.channels(), UINT8_MAX);
		assertNotNull(fileInfo.title());
		assertEqual(fileInfo.title(), title);
		assertEqual(fileInfo.titlePtr().get(), fileInfo.title());
		assertNotNull(fileInfo.artist());
		assertEqual(fileInfo.artist(), artist);
		assertEqual(fileInfo.artistPtr().get(), fileInfo.artist());
		assertNotNull(fileInfo.album());
		assertEqual(fileInfo.album(), album);
		assertEqual(fileInfo.albumPtr().get(), fileInfo.album());
	}

	void testFileInfoC()
	{
		assertEqual(audioFileTotalTime(nullptr), 0U);
		assertEqual(audioFileBitsPerSample(nullptr), 0U);
		assertEqual(audioFileBitRate(nullptr), 0U);
		assertEqual(audioFileChannels(nullptr), 0U);
		assertNull(audioFileTitle(nullptr));
		assertNull(audioFileArtist(nullptr));
		assertNull(audioFileAlbum(nullptr));

		fileInfo_t fileInfo{};
		assertEqual(audioFileTotalTime(&fileInfo), 0U);
		assertEqual(audioFileBitsPerSample(&fileInfo), 0U);
		assertEqual(audioFileBitRate(&fileInfo), 0U);
		assertEqual(audioFileChannels(&fileInfo), 0U);
		assertNull(audioFileTitle(&fileInfo));
		assertNull(audioFileArtist(&fileInfo));
		assertNull(audioFileAlbum(&fileInfo));

		fileInfo.totalTime(UINT64_MAX);
		fileInfo.bitsPerSample(UINT32_MAX);
		fileInfo.bitRate(UINT32_MAX);
		fileInfo.channels(UINT8_MAX);
		fileInfo.title(stringDup(title.data()));
		fileInfo.artist(stringDup(artist.data()));
		fileInfo.album(stringDup(album.data()));

		assertEqual(audioFileTotalTime(&fileInfo), UINT64_MAX);
		assertEqual(audioFileBitsPerSample(&fileInfo), UINT32_MAX);
		assertEqual(audioFileBitRate(&fileInfo), UINT32_MAX);
		assertEqual(audioFileChannels(&fileInfo), UINT8_MAX);
		assertNotNull(audioFileTitle(&fileInfo));
		assertEqual(audioFileTitle(&fileInfo), title);
		assertNotNull(audioFileArtist(&fileInfo));
		assertEqual(audioFileArtist(&fileInfo), artist);
		assertNotNull(audioFileAlbum(&fileInfo));
		assertEqual(audioFileAlbum(&fileInfo), album);
	}

public:
	void registerTests() final
	{
		CXX_TEST(testFileInfoCXX)
		CXX_TEST(testFileInfoC)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testFileInfo>();
}

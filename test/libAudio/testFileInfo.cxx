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
constexpr static auto other{"other"sv};

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
		assertEqual(fileInfo.otherCommentsCount(), 0);
		assertNull(fileInfo.otherComment(0));

		fileInfo.totalTime(UINT64_MAX);
		fileInfo.bitsPerSample(UINT8_MAX);
		fileInfo.bitRate(UINT32_MAX);
		fileInfo.channels(UINT8_MAX);
		fileInfo.title(stringDup(title.data()));
		fileInfo.artist(stringDup(artist.data()));
		fileInfo.album(stringDup(album.data()));
		fileInfo.addOtherComment(stringDup(other.data()));

		assertEqual(fileInfo.totalTime(), UINT64_MAX);
		assertEqual(fileInfo.bitsPerSample(), UINT8_MAX);
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
		assertEqual(fileInfo.otherCommentsCount(), 1);
		assertEqual(fileInfo.otherComment(0), other);
		assertNull(fileInfo.otherComment(1));
		const auto &otherComments{fileInfo.other()};
		assertEqual(otherComments.size(), 1);
		assertTrue(otherComments.front().get() == fileInfo.otherComment(0));
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
		assertEqual(audioFileOtherCommentsCount(nullptr), 0U);
		assertNull(audioFileOtherComment(nullptr, 0));

		fileInfo_t fileInfo{};
		assertEqual(audioFileTotalTime(&fileInfo), 0U);
		assertEqual(audioFileBitsPerSample(&fileInfo), 0U);
		assertEqual(audioFileBitRate(&fileInfo), 0U);
		assertEqual(audioFileChannels(&fileInfo), 0U);
		assertNull(audioFileTitle(&fileInfo));
		assertNull(audioFileArtist(&fileInfo));
		assertNull(audioFileAlbum(&fileInfo));
		assertEqual(audioFileOtherCommentsCount(&fileInfo), 0U);
		assertNull(audioFileOtherComment(&fileInfo, 0));

		fileInfo.totalTime(UINT64_MAX);
		fileInfo.bitsPerSample(UINT8_MAX);
		fileInfo.bitRate(UINT32_MAX);
		fileInfo.channels(UINT8_MAX);
		fileInfo.title(stringDup(title.data()));
		fileInfo.artist(stringDup(artist.data()));
		fileInfo.album(stringDup(album.data()));
		fileInfo.addOtherComment(stringDup(other.data()));

		assertEqual(audioFileTotalTime(&fileInfo), UINT64_MAX);
		assertEqual(audioFileBitsPerSample(&fileInfo), UINT8_MAX);
		assertEqual(audioFileBitRate(&fileInfo), UINT32_MAX);
		assertEqual(audioFileChannels(&fileInfo), UINT8_MAX);
		assertNotNull(audioFileTitle(&fileInfo));
		assertEqual(audioFileTitle(&fileInfo), title);
		assertNotNull(audioFileArtist(&fileInfo));
		assertEqual(audioFileArtist(&fileInfo), artist);
		assertNotNull(audioFileAlbum(&fileInfo));
		assertEqual(audioFileAlbum(&fileInfo), album);

		assertEqual(audioFileOtherCommentsCount(&fileInfo), 1);
		assertEqual(audioFileOtherComment(&fileInfo, 0), other);
		assertNull(audioFileOtherComment(&fileInfo, 1));
	}

public:
	void registerTests() final
	{
		CXX_TEST(testFileInfoCXX)
		CXX_TEST(testFileInfoC)
	}
};

CRUNCHpp_TESTS(testFileInfo)

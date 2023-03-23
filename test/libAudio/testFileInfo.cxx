#include <crunch++.h>
#include <libAudio.h>
#include <fileInfo.hxx>

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

		fileInfo.totalTime(UINT64_MAX);
		fileInfo.bitsPerSample(UINT32_MAX);
		fileInfo.bitRate(UINT32_MAX);
		fileInfo.channels(UINT8_MAX);

		assertEqual(fileInfo.totalTime(), UINT64_MAX);
		assertEqual(fileInfo.bitsPerSample(), UINT32_MAX);
		assertEqual(fileInfo.bitRate(), UINT32_MAX);
		assertEqual(fileInfo.channels(), UINT8_MAX);
	}

	void testFileInfoC()
	{
		assertEqual(audioFileTotalTime(nullptr), 0U);
		assertEqual(audioFileBitsPerSample(nullptr), 0U);
		assertEqual(audioFileBitRate(nullptr), 0U);
		assertEqual(audioFileChannels(nullptr), 0U);

		fileInfo_t fileInfo{};
		assertEqual(audioFileTotalTime(&fileInfo), 0U);
		assertEqual(audioFileBitsPerSample(&fileInfo), 0U);
		assertEqual(audioFileBitRate(&fileInfo), 0U);
		assertEqual(audioFileChannels(&fileInfo), 0U);

		fileInfo.totalTime(UINT64_MAX);
		fileInfo.bitsPerSample(UINT32_MAX);
		fileInfo.bitRate(UINT32_MAX);
		fileInfo.channels(UINT8_MAX);

		assertEqual(audioFileTotalTime(&fileInfo), UINT64_MAX);
		assertEqual(audioFileBitsPerSample(&fileInfo), UINT32_MAX);
		assertEqual(audioFileBitRate(&fileInfo), UINT32_MAX);
		assertEqual(audioFileChannels(&fileInfo), UINT8_MAX);
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

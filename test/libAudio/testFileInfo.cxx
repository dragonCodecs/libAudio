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
	}

	void testFileInfoC()
	{
		assertEqual(audioFileTotalTime(nullptr), 0U);
		assertEqual(audioFileBitsPerSample(nullptr), 0U);
		assertEqual(audioFileBitRate(nullptr), 0U);
		assertEqual(audioFileChannels(nullptr), 0U);
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

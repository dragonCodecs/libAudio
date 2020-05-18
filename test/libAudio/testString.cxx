#ifndef _WINDOWS
#include <unistd.h>
#else
#include <io.h>
#endif
#include <crunch++.h>
#include "testString.hxx"

class testString final : public testsuite
{
private:
	void testRawDup() { string::testRawDup(*this); }
	void testUniqueDup() { string::testUniqueDup(*this); }
	void testStringsLength() { string::testStringsLength(*this); }
	void testStringConcat() { string::testStringConcat(*this); }

public:
	void registerTests() final override
	{
		CXX_TEST(testRawDup)
		CXX_TEST(testUniqueDup)
		CXX_TEST(testStringsLength)
		CXX_TEST(testStringConcat)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testString>();
}

#ifndef _WINDOWS
#include <unistd.h>
#else
#include <io.h>
#endif
#include <crunch++.h>
#include "testFD.hxx"

class testBoundedIterator final : public testsuit
{
private:
	void testBadFD() { fd::testBadFD(*this); }
	void testBadOpen() { fd::testBadOpen(*this); }
	void testWrite() { fd::testWrite(*this); }
	void testSeek() { fd::testSeek(*this); }
	void testRead() { fd::testRead(*this); }

public:
	~testBoundedIterator() { unlink("fd.test"); }

	void registerTests() final override
	{
		CXX_TEST(testBadFD)
		CXX_TEST(testBadOpen)
		CXX_TEST(testWrite)
		CXX_TEST(testSeek)
		CXX_TEST(testRead)
	}
};

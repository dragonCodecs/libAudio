#ifndef TEST_FIXED_VECTOR__HXX
#define TEST_FIXED_VECTOR__HXX

namespace boundedIterator
{
	void testCtor(testsuit &suite);
	void testIndex(testsuit &suite);
	void testInc(testsuit &suite);
	void testDec(testsuit &suite);
}

namespace fixedVector
{
	void testInvalid(testsuit &suite);
	void testIndexing(testsuit &suite);
	void testSwap(testsuit &suite);
}

#endif /*TEST_FIXED_VECTOR__HXX*/

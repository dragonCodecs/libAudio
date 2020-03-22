#include <crunch++.h>
#include <string.hxx>
#include "testString.hxx"

static const std::string testString = "This is only a test";

namespace string
{
	void testRawDup(testsuit &suite)
	{
		suite.assertNull(stringDup(nullptr));
		auto result = stringDup(testString.data());
		suite.assertNotNull(result);
		suite.assertEqual(result.get(), testString.data(), testString.size());
	}

	void testUniqueDup(testsuit &suite)
	{
		std::unique_ptr<char []> stringPtr{};
		suite.assertNull(stringDup(stringPtr));
		stringPtr = stringDup(testString.data());
		auto result = stringDup(stringPtr);
		suite.assertNotNull(result);
		suite.assertEqual(result.get(), testString.data(), testString.size());
	}

	void testStringsLength(testsuit &suite)
	{
		suite.assertEqual(stringsLength(""), 0);
		suite.assertEqual(stringsLength("T"), 1);
		suite.assertEqual(stringsLength("The ", "quick"), 9);
		suite.assertEqual(stringsLength("The ", "quick ", "fox"), 13);
	}

	void testStringConcat(testsuit &suite)
	{
		const auto a = stringConcat("");
		suite.assertNotNull(a);
		suite.assertEqual(a.get(), "", 1);

		const auto b = stringConcat("The");
		suite.assertNotNull(b);
		suite.assertEqual(b.get(), "The", 4);

		const auto c = stringConcat("Hello", "world");
		suite.assertNotNull(c);
		suite.assertEqual(c.get(), "Helloworld", 11);

		const auto d = stringConcat("Goodbye", " cruel ", "world");
		suite.assertNotNull(d);
		suite.assertEqual(d.get(), "Goodbye cruel world", 20);
	}
}

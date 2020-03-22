#include <cstdint>
#include <cstring>
#include <memory>
#include <array>
#include <crunch++.h>
#include <substrate/fd>
#include "testFD.hxx"

using substrate::fd_t;

constexpr static std::array<char, 4> testArray{'t', 'E', 'S', 't'};
constexpr static char testChar{'.'};
static std::string testString{"fileDescriptor"};

constexpr static auto u8{uint8_t(0x5A)};
constexpr static auto i8{int8_t(0xA5)};
constexpr static auto u16{uint16_t(0x125A)};
constexpr static auto i16{int16_t(0x12A5)};
constexpr static auto u32{uint32_t(UINT32_C(0x1234565A))};
constexpr static auto i32{int32_t(INT32_C(0x123456A5))};
constexpr static auto u64{uint64_t(UINT64_C(0x123456789ABCDE5A))};
constexpr static auto i64{int64_t(INT64_C(0x123456789ABCDEA5))};

namespace fd
{
	void testBadFD(testsuit &suite)
	{
		fd_t file{};
		suite.assertEqual(file, -1);
		suite.assertFalse(file.valid());
		suite.assertFalse(file.isEOF());

		suite.assertEqual(file.read(nullptr, 0, nullptr), -1);
		suite.assertEqual(file.write(nullptr, 0, nullptr), -1);
	}

	void testBadOpen(testsuit &suite)
	{
		fd_t file{"bad.file", O_RDONLY};
		suite.assertEqual(file, -1);
		suite.assertFalse(file.valid());
		suite.assertFalse(file.isEOF());
	}

	std::unique_ptr<char []> toUnique(const std::string &value)
	{
		std::unique_ptr<char []> result{new char[value.size()]};
		memcpy(result.get(), value.data(), value.size());
		return result;
	}

	std::unique_ptr<char> toUnique(const char value)
	{
		std::unique_ptr<char> result{new char};
		*result = value;
		return result;
	}

	void testWrite(testsuit &suite)
	{
		fd_t file{"fd.test", O_WRONLY | O_CREAT | O_EXCL, substrate::normalMode};
		suite.assertGreaterThan(file, -1);
		suite.assertTrue(file.valid());
		suite.assertFalse(file.isEOF());

		suite.assertTrue(file.write(testArray));
		suite.assertTrue(file.write(testChar));
		auto arrPtr = toUnique(testString);
		suite.assertTrue(file.write(arrPtr, testString.size()));
		auto objPtr = toUnique(testChar);
		suite.assertTrue(file.write(objPtr));
		suite.assertTrue(file.write(u8));
		suite.assertTrue(file.write(i8));
		suite.assertTrue(file.writeLE(u16));
		suite.assertTrue(file.writeLE(i16));
		suite.assertTrue(file.writeLE(u32));
		suite.assertTrue(file.writeLE(i32));
		suite.assertTrue(file.writeLE(u64));
		suite.assertTrue(file.writeLE(i64));
		suite.assertTrue(file.writeBE(u16));
		suite.assertTrue(file.writeBE(i16));
		suite.assertTrue(file.writeBE(u32));
		suite.assertTrue(file.writeBE(i32));
		suite.assertTrue(file.writeBE(u64));
		suite.assertTrue(file.writeBE(i64));
	}

	void testSeek(testsuit &suite)
	{
		fd_t file{"fd.test", O_RDONLY};
		suite.assertTrue(file.valid());
		suite.assertEqual(file.tell(), 0);
		const off_t length = file.length();
		suite.assertEqual(length, 78);
		suite.assertTrue(file.tail());
		suite.assertEqual(file.tell(), length);
		suite.assertEqual(file.seek(-(length / 2), SEEK_CUR), length / 2);
		suite.assertEqual(file.seek(0, SEEK_END), length);
		suite.assertTrue(file.head());
		suite.assertFalse(file.isEOF());
	}

	void readUnique(testsuit &suite, const fd_t &file, const std::string &expected)
	{
		std::unique_ptr<char []> result{new char[expected.size()]};
		suite.assertNotNull(result);
		suite.assertTrue(file.read(result, expected.size()));
		suite.assertEqual(result.get(), expected.data(), expected.size());
		suite.assertFalse(file.isEOF());
	}

	void readUnique(testsuit &suite, const fd_t &file, const char expected)
	{
		std::unique_ptr<char> result{new char};
		suite.assertNotNull(result);
		suite.assertTrue(file.read(result));
		suite.assertEqual(*result, expected);
		suite.assertFalse(file.isEOF());
	}

	template<typename T> void read(testsuit &suite, const fd_t &file, const T &expected)
	{
		T result{};
		suite.assertTrue(file.read(result));
		suite.assertEqual(result, expected);
		suite.assertFalse(file.isEOF());
	}

	template<typename T, size_t N> void readArray(testsuit &suite, const fd_t &file,
		const std::array<T, N> &expected)
	{
		std::array<T, N> result{};
		suite.assertTrue(file.read(result));
		suite.assertEqual(result.data(), expected.data(), expected.size());
		suite.assertFalse(file.isEOF());
	}

	template<typename T> void readLE(testsuit &suite, const fd_t &file, const T expected)
	{
		T result{};
		suite.assertTrue(file.readLE(result));
		suite.assertEqual(result, expected);
		suite.assertFalse(file.isEOF());
	}

	template<typename T> void readBE(testsuit &suite, const fd_t &file, const T expected)
	{
		T result{};
		suite.assertTrue(file.readBE(result));
		suite.assertEqual(result, expected);
		suite.assertFalse(file.isEOF());
	}

	void testRead(testsuit &suite)
	{
		fd_t file{"fd.test", O_RDONLY};
		suite.assertGreaterThan(file, -1);
		suite.assertTrue(file.valid());
		suite.assertFalse(file.isEOF());

		readArray(suite, file, testArray);
		read(suite, file, testChar);
		readUnique(suite, file, testString);
		readUnique(suite, file, testChar);
		read(suite, file, u8);
		read(suite, file, i8);
		readLE(suite, file, u16);
		readLE(suite, file, i16);
		readLE(suite, file, u32);
		readLE(suite, file, i32);
		readLE(suite, file, u64);
		readLE(suite, file, i64);
		readBE(suite, file, u16);
		readBE(suite, file, i16);
		readBE(suite, file, u32);
		readBE(suite, file, i32);
		readBE(suite, file, u64);
		readBE(suite, file, i64);

		char junk{};
		suite.assertFalse(file.read(junk));
		suite.assertTrue(file.isEOF());
		suite.assertFalse(file.read(junk));
		suite.assertTrue(file.isEOF());
	}
}

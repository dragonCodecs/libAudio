// SPDX-License-Identifier: BSD-3-Clause
#ifndef STRING_HXX
#define STRING_HXX

#include "uniquePtr.hxx"

inline size_t stringsLength(const char *const part) noexcept { return strlen(part); }
template<typename... Args> inline size_t stringsLength(const char *const part, Args &&...args) noexcept
	{ return strlen(part) + stringsLength(args...); }

inline  std::unique_ptr<char []> stringsConcat(std::unique_ptr<char []> &&dst, const size_t offset, const char *const part) noexcept
{
	if (!dst)
		return nullptr;
	strcpy(dst.get() + offset, part);
	return std::move(dst);
}

template<typename... Args> inline std::unique_ptr<char []> stringsConcat(std::unique_ptr<char []> &&dst, const size_t offset,
	const char *const part, Args &&...args) noexcept
{
	if (!dst)
		return nullptr;
	strcpy(dst.get() + offset, part);
	return stringsConcat(std::move(dst), offset + strlen(part), args...);
}

template<typename... Args> inline std::unique_ptr<char []> stringConcat(const char *const part, Args &&...args) noexcept
	{ return stringsConcat(makeUnique<char []>(stringsLength(part, args...) + 1), 0, part, args...); }

inline void copyComment(std::unique_ptr<char []> &dst, const char *const src) noexcept
	{ dst = !dst ? stringConcat(src) : stringConcat(dst.get(), " / ", src); }

#endif /*STRING_HXX*/

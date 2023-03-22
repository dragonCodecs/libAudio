// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2019-2023 Rachel Mant <git@dragonmux.network>
#ifndef UNIQUE_PTR_HXX
#define UNIQUE_PTR_HXX

#include <memory>
#include <cstring>
#include <cstdlib>

template<typename T> struct makeUnique_ { using uniqueType = std::unique_ptr<T>; };
template<typename T> struct makeUnique_<T []> { using arrayType = std::unique_ptr<T []>; };
template<typename T, size_t N> struct makeUnique_<T [N]> { struct invalidType { }; };

template<typename T, typename... Args> inline typename makeUnique_<T>::uniqueType makeUnique(Args &&...args) noexcept(noexcept(T(std::forward<Args>(args)...)))
{
	using consT = typename std::remove_const<T>::type;
	return std::unique_ptr<T>(new (std::nothrow) consT(std::forward<Args>(args)...));
}

template<typename T> inline typename makeUnique_<T>::arrayType makeUnique(const size_t num) noexcept
{
	using consT = typename std::remove_const<typename std::remove_extent<T>::type>::type;
	return std::unique_ptr<T>(new (std::nothrow) consT[num]());
}

template<typename T, typename... Args> inline typename makeUnique_<T>::invalidType makeUnique(Args &&...) noexcept = delete;

template<typename T, typename... args_t> inline typename makeUnique_<T>::uniqueType makeUniqueT(args_t &&...args)
{
	using consT = typename std::remove_const<T>::type;
	return std::unique_ptr<T>{new consT(std::forward<args_t>(args)...)};
}

template<typename T> inline typename makeUnique_<T>::arrayType makeUniqueT(const size_t num)
{
	using consT = typename std::remove_const<typename std::remove_extent<T>::type>::type;
	return std::unique_ptr<T>{new consT[num]()};
}

template<typename T, typename... args_t> inline typename makeUnique_<T>::invalidType makeUniqueT(args_t &&...) noexcept = delete;

struct freeDelete final { void operator ()(void *ptr) noexcept { free(ptr); } };

#endif /*UNIQUE_PTR_HXX*/

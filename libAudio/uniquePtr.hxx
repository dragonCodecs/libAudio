#ifndef UNIQUE_PTR__HXX
#define UNIQUE_PTR__HXX

#include <memory>
#include <cstring>

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

inline std::unique_ptr<char []> stringDup(const char *const str)
{
	if (!str)
		return nullptr;
	const size_t length = strlen(str) + 1;
	auto result = makeUnique<char []>(length);
	if (!result)
		return nullptr;
	strncpy(result.get(), str, length);
	return result;
}

inline std::unique_ptr<char []> stringDup(const std::unique_ptr<char []> &str)
	{ return stringDup(str.get()); }

struct freeDelete final { void operator ()(void *ptr) noexcept { free(ptr); } };

#endif /*UNIQUE_PTR__HXX*/

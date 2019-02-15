#ifndef UNIQUE_PTR__HXX
#define UNIQUE_PTR__HXX

#include <memory>

template<typename T> struct makeUnique_ { using uniqueType = unique_ptr<T>; };
template<typename T> struct makeUnique_<T []> { using arrayType = unique_ptr<T []>; };
template<typename T, size_t N> struct makeUnique_<T [N]> { struct invalidType { }; };

template<typename T, typename... args_t> inline typename makeUnique_<T>::uniqueType makeUnique(args_t &&...args)
{
	using consT = typename std::remove_const<T>::type;
	return {new consT(std::forward<args_t>(args)...)};
}

template<typename T> inline typename makeUnique_<T>::arrayType makeUnique(const size_t num)
{
	using consT = typename std::remove_const<typename std::remove_extent<T>::type>::type;
	return {new consT[num]()};
}

template<typename T, typename... args_t> inline typename makeUnique_<T>::invalidType makeUnique(args_t &&...) noexcept = delete;

#endif /*UNIQUE_PTR__HXX*/

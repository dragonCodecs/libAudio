#ifndef OPAQUE_PTR__HXX
#define OPAQUE_PTR__HXX

using delete_t = void (*)(void *const);

template<typename T> struct opaquePtr_t final
{
private:
	T *ptr;
	delete_t deleteFunc;

public:
	constexpr opaquePtr_t() noexcept : ptr{nullptr}, deleteFunc{nullptr} { }
	opaquePtr_t(T *p, delete_t &&del) noexcept : ptr{p}, deleteFunc{del} { }
	opaquePtr_t(opaquePtr_t &&p) noexcept : opaquePtr_t{} { swap(p); }
	~opaquePtr_t() noexcept { if (deleteFunc) deleteFunc(ptr); }
	opaquePtr_t &operator =(opaquePtr_t &&p) noexcept { swap(p); return *this; }

	operator T &() const noexcept { return *ptr; }
	explicit operator T &&() const = delete;
	T &operator *() noexcept { return *ptr; }
	const T &operator *() const noexcept { return *ptr; }
	T *operator ->() noexcept { return ptr; }
	const T *operator ->() const noexcept { return ptr; }
	explicit operator bool() const noexcept { return ptr; }

	void swap(opaquePtr_t &p) noexcept
	{
		std::swap(ptr, p.ptr);
		std::swap(deleteFunc, p.deleteFunc);
	}

	opaquePtr_t(const opaquePtr_t &) = delete;
	opaquePtr_t &operator =(const opaquePtr_t &) = delete;
};

template<typename T> void swap(opaquePtr_t<T> &a, opaquePtr_t<T> &b) noexcept
	{ a.swap(b); }

template<typename T> inline static void del(void *const object)
{
	if (object)
		static_cast<T *>(object)->~T();
	operator delete(object);
}

template<typename T> struct makeOpaque_ { using uniqueType = opaquePtr_t<T>; };
template<typename T> struct makeOpaque_<T []> { using arrayType = opaquePtr_t<T []>; };
template<typename T, size_t N> struct makeOpaque_<T [N]> { struct invalidType { }; };

template<typename T, typename... Args> inline typename makeOpaque_<T>::uniqueType makeOpaque(Args &&...args)
{
	using consT = typename std::remove_const<T>::type;
	return opaquePtr_t<T>(new consT(std::forward<Args>(args)...), del<T>);
}

template<typename T> inline typename makeOpaque_<T>::arrayType makeOpaque(const size_t num)
{
	using consT = typename std::remove_const<typename std::remove_extent<T>::type>::type;
	return opaquePtr_t<T>(new consT[num](), del<T []>);
}

template<typename T, typename... Args> inline typename makeOpaque_<T>::invalidType makeOpaque(Args &&...) noexcept = delete;

#endif /*OPAQUE_PTR__HXX*/

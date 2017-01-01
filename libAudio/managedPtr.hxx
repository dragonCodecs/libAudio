#ifndef MANAGED_PTR__HXX
#define MANAGED_PTR__HXX

//#include <type_traits>
#include <utility>

namespace managedPtr
{
	using delete_t = void(void *const);

	/*template<typename T>
		typename std::enable_if<!std::is_array<T>::value>::type deletePtr(void *const objectPtr)
	{
		T *const object = reinterpret_cast<T *const>(objectPtr);
		delete object;
	}

	template<typename T>
		typename std::enable_if<std::is_array<T>::value>::type deletePtr(void *const objectPtr)
	{
		T *const object = reinterpret_cast<T *const>(objectPtr);
		delete [] object;
	}*/

	template<typename T> struct managedPtr_t final
	{
	protected:
		T *ptr;
		delete_t *del;
		friend struct managedPtr_t<void>;

	private:
		static void deletePtr(void *const objectPtr)
		{
			T *const object = reinterpret_cast<T *const>(objectPtr);
			delete object;
		}

	public:
		constexpr managedPtr_t() noexcept : ptr(nullptr), del(nullptr) { }
		constexpr managedPtr_t(T *obj) noexcept : ptr(obj), del(deletePtr) { }
		managedPtr_t(managedPtr_t &&ptr) noexcept : managedPtr_t() { swap(ptr); }
		~managedPtr_t() noexcept { if (del) del(ptr); }
		managedPtr_t &operator =(managedPtr_t &&ptr) noexcept { swap(ptr); return *this; }
		managedPtr_t &operator =(T *obj) noexcept { return *this = managedPtr_t(obj); }

		operator T &() const noexcept { return *ptr; }
		explicit operator T &&() const = delete;
		T &operator *() const noexcept { return *ptr; }
		T *operator ->() const noexcept { return ptr; }
		T *get() noexcept { return ptr; }
		T *get() const noexcept { return ptr; }
		explicit operator bool() const noexcept { return ptr; }

		void swap(managedPtr_t &p) noexcept
		{
			std::swap(ptr, p.ptr);
			std::swap(del, p.del);
		}

		managedPtr_t(const managedPtr_t &) = delete;
		managedPtr_t &operator =(const managedPtr_t &) = delete;
		managedPtr_t &operator =(const std::nullptr_t &) = delete;
	};

	template<> struct managedPtr_t<void> final
	{
	private:
		void *ptr;
		delete_t *del;

	public:
		constexpr managedPtr_t() noexcept : ptr(nullptr), del(nullptr) { }
		managedPtr_t(managedPtr_t &&ptr) noexcept : managedPtr_t() { swap(ptr); }
		template<typename T> managedPtr_t(managedPtr_t<T> &&p) noexcept : ptr(p.ptr), del(p.del) { p.ptr = nullptr; p.del = nullptr; }
		~managedPtr_t() noexcept { if (del) del(ptr); }
		managedPtr_t &operator =(managedPtr_t &&ptr) noexcept { swap(ptr); return *this; }

		template<typename T> managedPtr_t &operator =(T *obj) noexcept { return *this = managedPtr_t<T>(obj); }
		template<typename T> managedPtr_t &operator =(managedPtr_t<T> &&ptr) noexcept { return *this = managedPtr_t(std::move(ptr)); }

		void *operator ->() const noexcept { return ptr; }
		void *get() noexcept { return ptr; }
		template<typename T> T *get() noexcept { return reinterpret_cast<T *>(ptr); }
		void *get() const noexcept { return ptr; }
		template<typename T> T *get() const noexcept { return reinterpret_cast<T *const>(ptr); }
		explicit operator bool() const noexcept { return ptr; }

		void swap(managedPtr_t &p) noexcept
		{
			std::swap(ptr, p.ptr);
			std::swap(del, p.del);
		}

		managedPtr_t(const managedPtr_t &) = delete;
		managedPtr_t &operator =(const managedPtr_t &) = delete;
		managedPtr_t &operator =(const std::nullptr_t &) = delete;
	};
}

template<typename T> using managedPtr_t = managedPtr::managedPtr_t<T>;
//using managedPtr::managedPtr_t<void>;

#endif /*MANAGED_PTR__HXX*/

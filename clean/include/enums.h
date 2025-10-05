#pragma once
#include <type_traits>

namespace clean
{

	template<typename T>
		requires std::is_enum_v<T>
	inline constexpr bool contains_any(T value, T flags)
	{
		using U = std::underlying_type_t<T>;
		return (static_cast<U>(value) & static_cast<U>(flags)) != 0;
	}


	template<typename T>
		requires std::is_enum_v<T>
	inline constexpr T operator|(T a, T b)
	{
		using U = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<U>(a) | static_cast<U>(b));
	}

	template<typename T>
		requires std::is_enum_v<T>
	inline constexpr T operator&(T a, T b)
	{
		using U = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<U>(a) & static_cast<U>(b));
	}

	template<typename T>
		requires std::is_enum_v<T>
	inline constexpr T& operator|=(T& a, T b)
	{
		a = a | b;
		return a;
	}

	template<typename T>
		requires std::is_enum_v<T>
	constexpr bool contains(T a)
	{
		using U = std::underlying_type_t<T>;
		return static_cast<U>(a) != 0;
	}

	template <typename T>
		requires std::is_enum_v<T>
	inline constexpr T operator~(T a)
		{
		using U = std::underlying_type_t<T>;
		return static_cast<T>(~static_cast<U>(a));
	}

	template<typename T>
		requires std::is_enum_v<T>
	inline constexpr T remove_flags(T value, T flags)
	{
		using U = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<U>(value) & ~static_cast<U>(flags));
	}

	inline bool is_number(std::string_view i)
	{
		for (char c : i)
		{
			if (!std::isdigit(c))
				return false;
		}
		return !i.empty();
	}

}

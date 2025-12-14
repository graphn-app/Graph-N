#pragma once

namespace cast
{

static bool allow_size_cut = true;

template <typename ENUM>
constexpr ENUM to_enum(uint64_t value, bool no_throw = false)
{
	uint64_t first = cast::to_u64(ENUM::_first);
	uint64_t last = cast::to_u64(ENUM::_last);
	if (value < first || value > last)
	{
		if (!no_throw)
		{
			throw std::runtime_error("Value does not match to enum type ");
		}
		return ENUM::_none;
	}
	return static_cast<ENUM>(value);
}

template <typename PARAM, typename RETURN, typename std::enable_if<!std::is_floating_point<PARAM>::value>::type* = nullptr>
constexpr RETURN to(PARAM param)
{
	RETURN t = static_cast<RETURN>(param);
	if (PARAM(t) < param)
	{
		if (!allow_size_cut)
			throw std::runtime_error("The size was cut off");
	}
	return t;
}

template <typename PARAM, typename RETURN, typename std::enable_if<std::is_floating_point<PARAM>::value>::type* = nullptr>
constexpr RETURN to(PARAM param)
{
	RETURN t = static_cast<RETURN>(param);
	if (param < 0)
	{
		int64_t i_param = static_cast<long long>(param);
		if (int64_t(t) < i_param)
		{
			if (!allow_size_cut)
				throw std::runtime_error("Size is cutted");
		}
	}
	else
	{
		uint64_t u_param = static_cast<unsigned long long>(param);
		if (uint64_t(t) < u_param)
		{
			if (!allow_size_cut)
				throw std::runtime_error("Size is cutted");
		}
	}
	return t;
}

template <typename T>
constexpr char to_char(T param)
{
	return to<T, char>(param);
}

template <typename T>
constexpr wchar_t to_wchar(T param)
{
	return to<T, wchar_t>(param);
}

template <typename T>
constexpr byte to_u8(T param)
{
	return to<T, byte>(param);
}

template <typename T, typename std::enable_if<!std::is_floating_point<T>::value>::type* = nullptr>
constexpr int16_t to_i16(T param)
{
	return to<T, int16_t>(param);
}

template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
constexpr int16_t to_i16(T param)
{
	return to<T, int16_t>(param);
}

template <typename T, typename std::enable_if<!std::is_floating_point<T>::value>::type* = nullptr>
constexpr uint16_t to_u16(T param)
{
	return to<T, uint16_t>(param);
}

template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
constexpr uint16_t to_u16(T param)
{
	return to<T, uint16_t>(param);
}

template <typename T>
constexpr uint32_t to_u32(T param)
{
	return to<T, uint32_t>(param);
}

template <typename T>
constexpr int32_t to_i32(T param)
{
	return to<T, int32_t>(param);
}

template <typename T>
constexpr uint64_t to_u64(T param)
{
	return to<T, uint64_t>(param);
}

template <typename T>
constexpr uint64_t to_szt(T param)
{
	return to<T, size_t>(param);
}

template <typename T>
constexpr int64_t to_i64(T param)
{
	return to<T, int64_t>(param);
}

template <typename T>
constexpr float to_float(T param)
{
	return to<T, float>(param);
}

template <typename T>
constexpr double to_double(T param)
{
	return to<T, double>(param);
}

template <typename T>
constexpr gid to_gid(T param)
{
	return to<T, gid>(param);
}

};	// namespace cast

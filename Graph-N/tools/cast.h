#pragma once

namespace cast
{

inline bool skip_check = false;

template <typename DEST, typename SOURCE>
constexpr bool can_fit(SOURCE arg)
{
    if (skip_check)
        return true;

    if constexpr (std::is_same_v<DEST, SOURCE>)
    {
        (void)arg;
        return true;
    }
    
    else if constexpr (std::is_floating_point_v<SOURCE> && std::is_integral_v<DEST>)
    {
        if (!std::isfinite(arg))
            return false;

        return arg >= static_cast<SOURCE>(std::numeric_limits<DEST>::lowest()) &&
            arg < (static_cast<SOURCE>(std::numeric_limits<DEST>::max()) + 1.0);
    }
    
    else if constexpr (std::is_integral_v<SOURCE> && std::is_signed_v<SOURCE> && !std::is_signed_v<DEST>)
    {
        return arg >= 0 && static_cast<std::make_unsigned_t<SOURCE>>(arg) <= std::numeric_limits<DEST>::max();
    }

    else if constexpr (std::is_integral_v<SOURCE> && !std::is_signed_v<SOURCE> && std::is_signed_v<DEST>)
    {
        return arg <= static_cast<std::make_unsigned_t<DEST>>(std::numeric_limits<DEST>::max());
    }

    else
    {
        return arg >= std::numeric_limits<DEST>::lowest() && arg <= std::numeric_limits<DEST>::max();
    }
}

template <typename SOURCE, typename DEST>
constexpr DEST to(SOURCE param)
{
    if (!can_fit<DEST>(param))
    {
#ifdef _DEBUG
        DEBUG_CATCH;
        throw std::runtime_error("Data loss due to casting");
#endif
    }
    return static_cast<DEST>(param);
}

template <typename ENUM>
constexpr ENUM to_enum(uint64_t value)
{
    uint64_t first = static_cast<uint64_t>(ENUM::_first);
    uint64_t last = static_cast<uint64_t>(ENUM::_last);
    if (value < first || value > last)
    {
        DEBUG_CATCH;
        throw std::runtime_error("Value does not match to enum type");
    }
    return static_cast<ENUM>(value);
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

template <typename T>
constexpr int16_t to_i16(T param)
{
	return to<T, int16_t>(param);
}

template <typename T>
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
    if constexpr (std::is_enum_v<T>)
    {
        return static_cast<int32_t>(param);
    }
    else
    {
        return to<T, int32_t>(param);
    }
}

template <typename T>
constexpr uint64_t to_u64(T param)
{
    if constexpr (std::is_enum_v<T>)
    {
        return static_cast<uint64_t>(param);
    }
    else
    {
        return to<T, uint64_t>(param);
    }
}

template <typename T>
constexpr int64_t to_i64(T param)
{
    if constexpr (std::is_enum_v<T>)
    {
        return static_cast<int64_t>(param);
    }
    else
    {
        return to<T, int64_t>(param);
    }
}

template <typename T>
constexpr size_t to_szt(T param)
{
    return to<T, size_t>(param);
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

};	// namespace cast

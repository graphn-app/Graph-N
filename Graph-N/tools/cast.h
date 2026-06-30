#pragma once

namespace cast
{

template <typename DEST, typename SOURCE>
constexpr bool can_fit(SOURCE arg)
{
    (void)arg;
    using S_Type = std::decay_t<SOURCE>;
    using D_Type = std::decay_t<DEST>;

    if constexpr (std::is_same_v<D_Type, S_Type>)
    {
        return true;
    }

    // double->float
    else if constexpr (std::is_floating_point_v<S_Type> && std::is_floating_point_v<D_Type>)
    {
        if (!std::isfinite(arg)) 
            return false;
        if constexpr (sizeof(D_Type) >= sizeof(S_Type))
            return true;
        return arg >= std::numeric_limits<D_Type>::lowest() && arg <= std::numeric_limits<D_Type>::max();
    }

    // Float -> Integer
    else if constexpr (std::is_floating_point_v<S_Type> && std::is_integral_v<D_Type>)
    {
        if (!std::isfinite(arg))
            return false;

        if constexpr (std::is_unsigned_v<D_Type>)
        {
            if (arg < 0)
                return false;
        }
        else
        {
            if (arg < static_cast<S_Type>(std::numeric_limits<D_Type>::lowest()))
                return false;
        }

        if (arg > static_cast<S_Type>(std::numeric_limits<D_Type>::max()))
            return false;

        return true;
    }

    // Integer -> Float
    else if constexpr (std::is_integral_v<S_Type> && std::is_floating_point_v<D_Type>)
    {
        return true; 
    }

    // Integer -> Integer
    else if constexpr (std::is_integral_v<S_Type> && std::is_integral_v<D_Type>)
    {
        // Signed -> Unsigned
        if constexpr (std::is_signed_v<S_Type> && !std::is_signed_v<D_Type>)
        {
            return arg >= 0 && static_cast<std::make_unsigned_t<S_Type>>(arg) <= std::numeric_limits<D_Type>::max();
        }
        // Unsigned -> Signed
        else if constexpr (!std::is_signed_v<S_Type> && std::is_signed_v<D_Type>)
        {
            return arg <= static_cast<std::make_unsigned_t<D_Type>>(std::numeric_limits<D_Type>::max());
        }

        else
        {
            return arg >= std::numeric_limits<D_Type>::lowest() && arg <= std::numeric_limits<D_Type>::max();
        }
    }
    else
    {
        // eczotic
        return true;
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

template <typename ENUM, typename T>
constexpr ENUM to_enum(T value)
{
    static_assert(std::is_enum_v<ENUM>, "Target type must be an enum");
    using Underlying = std::underlying_type_t<ENUM>;

    if (!can_fit<Underlying>(value))
    {
        DEBUG_CATCH;
        throw std::runtime_error("Value out of bounds for enum underlying type");
    }

    const Underlying val = static_cast<Underlying>(value);
    if (val < static_cast<Underlying>(ENUM::_first) || val > static_cast<Underlying>(ENUM::_last))
    {
        DEBUG_CATCH;
        throw std::runtime_error("Value is outside explicit ENUM range");
    }

    return static_cast<ENUM>(value);
}

template <typename T>
constexpr char to_char(T param)
{
    return to<T, char>(param);
}

template <typename T>
constexpr unsigned char to_uchar(T param)
{
    if constexpr (std::is_same_v<std::decay_t<T>, char> || std::is_same_v<std::decay_t<T>, signed char>)
        return static_cast<unsigned char>(param);
    else
        return to<T, unsigned char>(param);
}

template <typename T>
constexpr wchar_t to_wchar(T param)
{
    return to<T, wchar_t>(param);
}

template <typename T>
constexpr void* to_void(T param)
{
    return reinterpret_cast<void*>(std::uintptr_t(param));
}

template <typename T>
constexpr std::uintptr_t to_ptr(const T* param)
{
    return std::uintptr_t(param);
}

template <typename T>
constexpr const T* ptrs(const void* param)
{
    return reinterpret_cast<const T*>(param);
}

template <typename T>
constexpr T* ptrs(void* param)
{
    return reinterpret_cast<T*>(param);
}

template <typename T>
constexpr T* ptrs(const std::uintptr_t param)
{
    return reinterpret_cast<T*>(param);
}

template <typename T>
constexpr T from_ptr(const void* param)
{
    return static_cast<T>(to_ptr(param));
}

template <typename DEST, typename SRC>
constexpr DEST dynamic(SRC* param)
{
    return dynamic_cast<DEST>(param);
}

#define CAST_HELPER(NAME, TYPE) \
    template <typename T> \
    constexpr TYPE NAME(T param) { \
        if constexpr (std::is_enum_v<T>) return static_cast<TYPE>(param); \
        else return to<T, TYPE>(param); \
    }

CAST_HELPER(to_u8,  uint8_t)
CAST_HELPER(to_i16, int16_t)
CAST_HELPER(to_u16, uint16_t)
CAST_HELPER(to_i32, int32_t)
CAST_HELPER(to_u32, uint32_t)
CAST_HELPER(to_i64, int64_t)
CAST_HELPER(to_u64, uint64_t)
CAST_HELPER(to_szt, size_t)
CAST_HELPER(to_float, float)
CAST_HELPER(to_double, double)

#undef CAST_HELPER

}; // namespace cast

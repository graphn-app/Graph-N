#pragma once

namespace cast
{

template <typename DEST, typename SOURCE>
constexpr bool can_fit(SOURCE arg) noexcept
{
    using S_Type = std::decay_t<SOURCE>;
    using D_Type = std::decay_t<DEST>;

    if constexpr (std::same_as<D_Type, S_Type>)
    {
        return true;
    }
    // Integer -> Integer
    else if constexpr (std::integral<S_Type> && std::integral<D_Type>)
    {
        if constexpr (std::same_as<D_Type, bool>)
        {
            return arg == 0 || arg == 1;
        }
        else if constexpr (std::same_as<S_Type, bool>)
        {
            return true; // A boolean 0 or 1 will fit into any other integer type
        }
        else
        {
            // std::in_range strictly requires standard integer types.
            // plain char, wchar_t, etc., are std::integral but NOT standard integers.
            // map them to their standard equivalents (e.g., char -> signed/unsigned char).
            using Std_D_Type = std::conditional_t<std::is_signed_v<D_Type>, std::make_signed_t<D_Type>, std::make_unsigned_t<D_Type>>;
            using Std_S_Type = std::conditional_t<std::is_signed_v<S_Type>, std::make_signed_t<S_Type>, std::make_unsigned_t<S_Type>>;

            return std::in_range<Std_D_Type>(static_cast<Std_S_Type>(arg));
        }
    }
    // Float -> Float
    // Float -> Float
    else if constexpr (std::floating_point<S_Type> && std::floating_point<D_Type>)
    {
        if (!std::isfinite(arg))
            return false;

        if constexpr (sizeof(D_Type) >= sizeof(S_Type))
        {
            // Upcasting or same size casting is always safe
            return true;
        }
        else
        {
            // Downcasting requires a bounds check
            return arg >= static_cast<S_Type>(std::numeric_limits<D_Type>::lowest()) && 
                arg <= static_cast<S_Type>(std::numeric_limits<D_Type>::max());
        }
    }
    // Float -> Integer
    else if constexpr (std::floating_point<S_Type> && std::integral<D_Type>)
    {
        if (!std::isfinite(arg))
            return false;

        if constexpr (std::unsigned_integral<D_Type>)
        {
            if (arg < 0.0)
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
    else if constexpr (std::integral<S_Type> && std::floating_point<D_Type>)
    {
        return true; 
    }
    else
    {
        // Exotic
        return true;
    }
}

template <typename DEST, typename SOURCE>
constexpr DEST to(SOURCE param) noexcept
{
    if (!can_fit<DEST>(param))
    {
#ifdef _DEBUG
        ERROR_MESSAGE("Can't convert types");
        return {};
#endif
    }
    return static_cast<DEST>(param);
}

template <typename ENUM, typename T>
constexpr ENUM to_enum(T value) noexcept
{
    static_assert(std::is_enum_v<ENUM>, "Target type must be an enum");
    using Underlying = std::underlying_type_t<ENUM>;

    if (!can_fit<Underlying>(value))
    {
#ifdef _DEBUG
        ERROR_MESSAGE("Value out of bounds for enum underlying type");
#endif
        return ENUM::_none;
    }

    const Underlying val = static_cast<Underlying>(value);
    if (val < static_cast<Underlying>(ENUM::_first) || val > static_cast<Underlying>(ENUM::_last))
    {
#ifdef _DEBUG
        ERROR_MESSAGE("Value is outside explicit ENUM range");
#endif
        return ENUM::_none;
    }

    return static_cast<ENUM>(value);
}

template <typename T>
constexpr char to_char(T param) noexcept
{
    return to<char>(param);
}

template <typename T>
constexpr unsigned char to_uchar(T param) noexcept
{
    if constexpr (std::same_as<std::decay_t<T>, char> || std::same_as<std::decay_t<T>, signed char>)
        return static_cast<unsigned char>(param);
    else
        return to<unsigned char>(param);
}

template <typename T>
constexpr wchar_t to_wchar(T param) noexcept
{
    return to<wchar_t>(param);
}

template <typename T>
constexpr void* to_void(T param) noexcept
{
    if constexpr (std::is_pointer_v<T>)
    {
        return const_cast<void*>(static_cast<const volatile void*>(param));
    }
    else
    {
        return reinterpret_cast<void*>(static_cast<std::uintptr_t>(param));
    }
}

template <typename T>
constexpr std::uintptr_t to_ptr(const T* param) noexcept
{
    return std::uintptr_t(param);
}

template <typename T>
constexpr const T* ptrs(const void* param) noexcept
{
    return reinterpret_cast<const T*>(param);
}

template <typename T>
constexpr T* ptrs(void* param) noexcept
{
    return reinterpret_cast<T*>(param);
}

template <typename T>
constexpr T* ptrs(const std::uintptr_t param) noexcept
{
    return reinterpret_cast<T*>(param);
}

template <typename T>
constexpr T from_ptr(const void* param) noexcept
{
    return static_cast<T>(reinterpret_cast<std::uintptr_t>(param));
}

// dynamic_cast is legal in constant expressions since C++20 (for polymorphic types)
template <typename DEST, typename SRC>
constexpr DEST* dynamic(SRC* param) noexcept
{
    return dynamic_cast<DEST*>(param);
}

template <typename DEST, typename SRC>
constexpr const DEST* dynamic(const SRC* param) noexcept
{
    return dynamic_cast<const DEST*>(param);
}

template <typename TYPE>
constexpr TYPE* remove_const(const TYPE* type) noexcept
{
    return const_cast<TYPE*>(type);
}

#define CAST_HELPER(NAME, TYPE) \
    template <typename T> \
    constexpr TYPE NAME(T param) noexcept { \
        if constexpr (std::is_enum_v<T>) return static_cast<TYPE>(param); \
        else return to<TYPE>(param); \
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
CAST_HELPER(to_imguikey, ImGuiKey)

#undef CAST_HELPER

}; // namespace cast

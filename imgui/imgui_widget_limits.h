// dear imgui, v1.89.9
// (widget limits)

#pragma once

// Those MIN/MAX values are not define because we need to point to them
constexpr const signed char    IM_S8_MIN  = -128;
constexpr const signed char    IM_S8_MAX  = 127;
constexpr const unsigned char  IM_U8_MIN  = 0;
constexpr const unsigned char  IM_U8_MAX  = 0xFF;
constexpr const signed short   IM_S16_MIN = -32768;
constexpr const signed short   IM_S16_MAX = 32767;
constexpr const unsigned short IM_U16_MIN = 0;
constexpr const unsigned short IM_U16_MAX = 0xFFFF;
constexpr const ImS32          IM_S32_MIN = INT_MIN;    // (-2147483647 - 1), (0x80000000);
constexpr const ImS32          IM_S32_MAX = INT_MAX;    // (2147483647), (0x7FFFFFFF)
constexpr const ImU32          IM_U32_MIN = 0;
constexpr const ImU32          IM_U32_MAX = UINT_MAX;   // (0xFFFFFFFF)
#ifdef LLONG_MIN
constexpr const ImS64          IM_S64_MIN = LLONG_MIN;  // (-9223372036854775807ll - 1ll);
constexpr const ImS64          IM_S64_MAX = LLONG_MAX;  // (9223372036854775807ll);
#else
constexpr const ImS64          IM_S64_MIN = -9223372036854775807LL - 1;
constexpr const ImS64          IM_S64_MAX = 9223372036854775807LL;
#endif
constexpr const ImS64          IM_S64_MIN_WIDGET = IM_S64_MIN / 2;
constexpr const ImS64          IM_S64_MAX_WIDGET = IM_S64_MAX / 2;
constexpr const ImU64          IM_U64_MIN = 0;
#ifdef ULLONG_MAX
constexpr const ImU64          IM_U64_MAX = ULLONG_MAX; // (0xFFFFFFFFFFFFFFFFull);
#else
constexpr const ImU64          IM_U64_MAX = (2ULL * 9223372036854775807LL + 1);
#endif
constexpr const double         IM_DBL_MIN_WIDGET = -DBL_MAX / 2.0;
constexpr const double         IM_DBL_MAX_WIDGET = DBL_MAX / 2.0;

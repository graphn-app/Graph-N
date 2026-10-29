/*
*   Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

using app_version_type  = uint32_t;

namespace ver
{

/* app main version (breaking changes) */
inline constexpr uint32_t app_major = 0;
/* app version (new backward-compatible features) */
inline constexpr uint32_t app_minor = 7;
/* app patch (bugfixes) */
inline constexpr uint32_t app_patch = 2;

/* date of release commit (for UI only) */
inline constexpr uint32_t release_year = 2026;
inline constexpr uint32_t release_month = 11;
inline constexpr uint32_t release_day = 3;

inline constexpr bool beta = true;

/* pack into 32-bit integer: 8 bits Major, 8 bits Minor, 16 bits Patch */
constexpr app_version_type build_version(uint32_t major, uint32_t minor, uint32_t patch) noexcept
{
    return (major << 24) | (minor << 16) | patch;
}

app_version_type from_string(const std::string& version) noexcept;

/* current version */
inline constexpr app_version_type current = build_version(app_major, app_minor, app_patch);

/* version extraction helpers */
constexpr uint32_t get_app_major(app_version_type ver = current) noexcept { return (ver >> 24) & 0xFF; }
constexpr uint32_t get_app_minor(app_version_type ver = current) noexcept { return (ver >> 16) & 0xFF; }
constexpr uint32_t get_app_patch(app_version_type ver = current) noexcept { return ver & 0xFFFF; }

/* strings for UI */
std::string ver_as_string(const std::string& separator = ".", app_version_type app_ver = current);
std::string ver_as_caption();

/* compatibility checks */
constexpr bool is_compatible(app_version_type package_ver) noexcept
{
    // typically, major version dictates breaking changes
    // if major matches, the logic should be compatible
    return 0 == package_ver || get_app_major(package_ver) == app_major;
}

constexpr bool is_greater(app_version_type new_ver) noexcept
{
    return new_ver > current;
}

} // namespace ver

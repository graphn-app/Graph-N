/*
*   Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace ver
{

inline constexpr uint64_t app_major = 1;
/* app version, some big changes */
inline constexpr uint64_t app_minor = 0;
/* app patch, small changes */
inline constexpr uint64_t app_patch = 0;

/* DB version */
inline constexpr uint64_t db_major = 1;
/* DB patch, small compatible changes */
inline constexpr uint64_t db_patch = 0;

/* date of release commit */
inline constexpr uint64_t release_year = 25;
inline constexpr uint64_t release_month = 11;
inline constexpr uint64_t release_day = 26;

constexpr app_version_type build_version(uint64_t app_major_, uint64_t app_minor_, uint64_t app_patch_, uint64_t db_major_, uint64_t db_patch_, uint64_t year_, uint64_t month_, uint64_t day_) noexcept
{
    return
        (app_major_ << 56) | (app_minor_ << 48) | (app_patch_ << 40) |
        (db_major_ << 32) | (db_patch_ << 24) |
        (year_ << 16) | (month_ << 8) | day_
    ;
}

/* current version */
constexpr app_version_type current = build_version(app_major, app_minor, app_patch, db_major, db_patch, release_year, release_month, release_day);

std::string ver_as_string(const std::string& separator = ".", app_version_type app_ver = current);

constexpr byte get_app_major(app_version_type app_ver = current) noexcept;
constexpr byte get_app_minor(app_version_type app_ver = current) noexcept;
constexpr byte get_app_patch(app_version_type app_ver = current) noexcept;
constexpr byte get_db_major(app_version_type app_ver = current) noexcept;
constexpr byte get_db_patch(app_version_type app_ver = current) noexcept;

constexpr bool is_compatible(app_version_type package_ver) noexcept;

} // namespace ver

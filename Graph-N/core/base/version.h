#pragma once

namespace ver
{
/*
1.0.0.0
	- initial release

!!
    - update version in resource file
    - build with RelWithDebInfo
    - save release .exe/.pdb files
!!
*/

/* main APP version */
static constexpr uint64_t app_major = 1;
/* app version, some big changes */
static constexpr uint64_t app_minor = 0;
/* app patch, small changes */
static constexpr uint64_t app_patch = 0;

/* DB version */
static constexpr uint64_t db_major = 1;
/* DB patch, small compatible changes */
static constexpr uint64_t db_patch = 0;

/* date of release commit */
static constexpr uint64_t release_year = 25;
static constexpr uint64_t release_month = 11;
static constexpr uint64_t release_day = 26;

app_version_type build_version(uint64_t app_major_, uint64_t app_minor_, uint64_t app_patch_, uint64_t db_major_, uint64_t db_patch_, uint64_t year_, uint64_t month_, uint64_t day_);

/* current version */
static const app_version_type current = build_version(app_major, app_minor, app_patch, db_major, db_patch, release_year, release_month, release_day);

std::string ver_as_string(const std::string& separator = ".", app_version_type app_ver = current);
std::string ver_as_string_no_prepended_zeros(const std::string& separator = ".", app_version_type app_ver = current);
std::string ver_as_string_extended(app_version_type app_ver = current);

byte get_app_major(app_version_type app_ver = current);
byte get_app_minor(app_version_type app_ver = current);
byte get_app_patch(app_version_type app_ver = current);
byte get_db_major(app_version_type app_ver = current);
byte get_db_patch(app_version_type app_ver = current);
uint64_t get_release_date(app_version_type app_ver = current);

bool is_compatible(app_version_type package_ver);

} // namespace ver

#include "core/base/version.h"

namespace ver
{
constexpr void version_to_parts(
    app_version_type ver, byte& app_major_, byte& app_minor_, byte& app_patch_, byte& db_major_, byte& db_patch_, byte& year_, byte& month_, byte& day_) noexcept
{
    app_major_ = (ver >> 56) & 0xff;
    app_minor_ = (ver >> 48) & 0xff;
    app_patch_ = (ver >> 40) & 0xff;
    db_major_ = (ver >> 32) & 0xff;
    db_patch_ = (ver >> 24) & 0xff;
    year_ = (ver >> 16) & 0xff;
    month_ = (ver >> 8) & 0xff;
    day_ = (ver & 0xff);
}

std::string ver_as_string(const std::string& separator /*= "."*/, app_version_type app_ver /*= current*/)
{
    byte app_major_ = 0, app_minor_ = 0, app_patch_ = 0, db_major_ = 0, db_patch_ = 0, year_ = 0, month_ = 0, day_ = 0;
    version_to_parts(app_ver, app_major_, app_minor_, app_patch_, db_major_, db_patch_, year_, month_, day_);

    return fmt::format(FMT_STRING("{}{}{:02}{}{:02}{}{:02}{}{:02}{}{:02}{:02}{:02}"),
        cast::to_u32(app_major_), separator,
        cast::to_u32(app_minor_), separator,
        cast::to_u32(app_patch_), separator,
        cast::to_u32(db_major_), separator,
        cast::to_u32(db_patch_), separator,
        cast::to_u32(year_),
        cast::to_u32(month_),
        cast::to_u32(day_)
    );
}

constexpr byte get_app_major(app_version_type app_ver /*= current*/) noexcept
{
    byte app_major_ = 0, app_minor_ = 0, app_patch_ = 0, db_major_ = 0, db_patch_ = 0, year_ = 0, month_ = 0, day_ = 0;
    version_to_parts(app_ver, app_major_, app_minor_, app_patch_, db_major_, db_patch_, year_, month_, day_);
    return app_major_;
}

constexpr byte get_app_minor(app_version_type app_ver /*= current*/) noexcept
{
    byte app_major_ = 0, app_minor_ = 0, app_patch_ = 0, db_major_ = 0, db_patch_ = 0, year_ = 0, month_ = 0, day_ = 0;
    version_to_parts(app_ver, app_major_, app_minor_, app_patch_, db_major_, db_patch_, year_, month_, day_);
    return app_minor_;
}

constexpr byte get_app_patch(app_version_type app_ver /*= current*/) noexcept
{
    byte app_major_ = 0, app_minor_ = 0, app_patch_ = 0, db_major_ = 0, db_patch_ = 0, year_ = 0, month_ = 0, day_ = 0;
    version_to_parts(app_ver, app_major_, app_minor_, app_patch_, db_major_, db_patch_, year_, month_, day_);
    return app_patch_;
}

constexpr byte get_db_major(app_version_type app_ver /*= current*/) noexcept
{
    byte app_major_ = 0, app_minor_ = 0, app_patch_ = 0, db_major_ = 0, db_patch_ = 0, year_ = 0, month_ = 0, day_ = 0;
    version_to_parts(app_ver, app_major_, app_minor_, app_patch_, db_major_, db_patch_, year_, month_, day_);
    return db_major_;
}

constexpr byte get_db_patch(app_version_type app_ver /*= current*/) noexcept
{
    byte app_major_ = 0, app_minor_ = 0, app_patch_ = 0, db_major_ = 0, db_patch_ = 0, year_ = 0, month_ = 0, day_ = 0;
    version_to_parts(app_ver, app_major_, app_minor_, app_patch_, db_major_, db_patch_, year_, month_, day_);
    return db_patch_;
}

constexpr bool is_compatible(app_version_type package_ver) noexcept
{
    const auto package_db_major = get_db_major(package_ver);
    const auto package_app_major = get_app_major(package_ver);
    return (package_db_major == db_major && package_app_major == app_major);
}

} // namespace ver

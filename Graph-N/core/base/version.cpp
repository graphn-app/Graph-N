/*
*   Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/base/version.h"

namespace ver
{

app_version_type from_string(const std::string& version) noexcept
{
    uint32_t major = 0, minor = 0, patch = 0;

    const char* str = version.c_str();
    if (*str == 'v' || *str == 'V')
        ++str;

    if (std::sscanf(str, "%u.%u.%u", &major, &minor, &patch) != 3)
        return 0;

    // build_version() packs 8/8/16 bits; the string comes from the network, so don't assert - just reject
    if (major > 0xFF || minor > 0xFF || patch > 0xFFFF)
        return 0;

    return ver::build_version(major, minor, patch);
}

std::string ver_as_string(const std::string& separator /*= "."*/, app_version_type app_ver /*= current*/)
{
    return fmt::format(FMT_STRING("{}{}{}{}{}"),
        get_app_major(app_ver), separator,
        get_app_minor(app_ver), separator,
        get_app_patch(app_ver)
    );
}

std::string ver_as_caption()
{
    return fmt::format(FMT_STRING("{}.{}.{} {}"),
        app_major,
        app_minor,
        app_patch,
        beta ? "beta" : ""
    );
}

} // namespace ver

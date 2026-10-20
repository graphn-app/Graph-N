/*
*   Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/base/version.h"

namespace ver
{

app_version_type from_string(const std::string& version) noexcept
{
    uint32_t major = 0, minor = 0, patch = 0;

    // try to parse format "vX.Y.Z" or "vX.Y.Z-beta"
    if (std::sscanf(version.c_str(), "v%d.%d.%d", &major, &minor, &patch) == 3 ||
        std::sscanf(version.c_str(), "v%d.%d.%d-beta", &major, &minor, &patch) == 3)
    {
        return ver::build_version(major, minor, patch);
    }
    return 0;
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

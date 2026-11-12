/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#include "core/base/filesystem.h"
#if defined __linux__ || defined __APPLE__
#include <unistd.h>
#include <limits.h>
#endif

namespace fs
{

std::filesystem::path get_this_app_filepath()
{
#ifdef _WIN32
    std::vector<wchar_t> buff(MAX_PATH);

    while (true)
    {
        const DWORD length = ::GetModuleFileNameW(nullptr, buff.data(), cast::to_u32(buff.size()));
        LogCriticalIfS(length > 0, "GetModuleFileNameW returned 0. Err " << LogApiERR);

        if (length < buff.size())
            return std::filesystem::path(buff.data());

        buff.resize(buff.size() * 2);
    }
    return {};

#elif defined __linux__ || defined __APPLE__
    std::error_code ec;
    std::filesystem::path p = std::filesystem::read_symlink("/proc/self/exe", ec); // note: /proc/self/exe exists on Linux only
    LogCriticalIfS(!ec, "std::filesystem::read_symlink(/proc/self/exe) failed. Err " << ec.value() << ": " << ec.message());
    return p;
#endif
}

std::filesystem::path get_root_directory_path()
{
    auto path = get_this_app_filepath();
    return path.remove_filename();
}

std::filesystem::path get_log_directory_path()
{
    auto path = get_root_directory_path();
    path /= "log";
    return path;
}

std::filesystem::path get_res_directory_path()
{
    auto path = get_root_directory_path();
    path /= "res";
    return path;
}

std::filesystem::path get_temp_directory_path()
{
    auto path = get_root_directory_path();
    path /= "temp";
    return path;
}

std::filesystem::path get_temp_filepath(const std::string& ext)
{
    const auto dir = get_temp_directory_path();
    if (!tools::path::make_folder(dir))
        return {};

    auto path = dir / (tools::str::generate_uuid_v4() + ext);
    while(tools::path::exists(path))
    {
        path = dir / (tools::str::generate_uuid_v4() + ext);
    }
    return path;
}

std::string sanitize_media_ext(const std::string& ext)
{
    // extension comes from a server-provided filename: allow only known media/text types, anything else becomes .bin
    static const std::unordered_set<std::string> allowed =
    {
        ".png", ".jpg", ".jpeg", ".webp", ".gif", ".bmp", ".tif", ".tiff", ".svg", ".ico", ".avif", ".apng",
        ".mp4", ".webm", ".mkv", ".avi", ".mov", ".m4v",
        ".wav", ".mp3", ".flac", ".ogg", ".opus", ".m4a", ".aac",
        ".glb", ".gltf", ".obj", ".fbx", ".stl", ".ply", ".usdz",
        ".json", ".txt", ".csv", ".md", ".image"
    };

    const auto lower = tools::str::to_lower_ascii(ext);
    if (allowed.contains(lower))
        return lower;
    return ".bin";
}

std::filesystem::path get_backup_directory_path()
{
    auto path = get_root_directory_path();
    path /= "backup";
    return path;
}

std::filesystem::path get_crash_dump_directory_path()
{
    auto path = get_root_directory_path();
    path /= "crash_dump";
    return path;
}

std::filesystem::path get_host_path()
{
    auto path = get_root_directory_path();
    path /= "Graph-N_Host.exe";
    return path;
}

std::filesystem::path get_host_webprofile_path(uint64_t host_id)
{
    auto path = get_temp_directory_path();
    if (0 == host_id)
        path /= "webprofile_debug";
    else
        path /= std::to_string(host_id);
    return path;
}

std::filesystem::path get_app_icon_path(bool png)
{
    auto path = get_res_directory_path();
    if (png)    path /= "graph-n.png";
    else        path /= "graph-n.ico";
    return path;
}

void get_license_paths(std::filesystem::path& license, std::filesystem::path& third_party, std::filesystem::path& privacy, std::filesystem::path& readme)
{
    auto root = get_root_directory_path();
    license =       root / "EULA.txt";
    third_party =   root / "THIRD-PARTY-NOTICES.txt";
    privacy =       root / "PRIVACY.md";
    readme =        root / "README.md";
}

}

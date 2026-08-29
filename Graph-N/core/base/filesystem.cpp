#include "core/base/filesystem.h"

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
#include <unistd.h>
#include <limits.h>	
    std::error_code ec;
    std::filesystem::path p = std::filesystem::read_symlink("/proc/self/exe", ec);
    LogCriticalIfS(!ec, "std::filesystem::read_symlink(/proc/self/exe) failed. Err " << ex.value() << ": " << err.message());    
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

}

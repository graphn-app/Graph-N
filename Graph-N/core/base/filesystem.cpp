
namespace fs
{

std::filesystem::path get_this_app_filepath()
{
#ifdef _WIN32
    std::vector<wchar_t> buff(MAX_PATH);

    while (true)
    {
        const DWORD length = GetModuleFileNameW(NULL, buff.data(), cast::to_u32(buff.size()));
        if (0 == length)
        {
            LogAndThrowS(ExcepCategory::API, "Failed to call WinAPI");
        }

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
    if (ec)
    {
        LogAndThrow(ExcepCategory::API, "Failed to call Unix API");
    }
    return p;
#endif
}

std::filesystem::path get_root_directory_path()
{
    auto root = get_this_app_filepath();
    return root.remove_filename();
}

std::filesystem::path get_log_directory_path()
{
    auto root = get_root_directory_path();
    root /= "log";
    return root;
}

std::filesystem::path get_res_directory_path()
{
    auto root = get_root_directory_path();
    root /= "res";
    return root;
}

std::filesystem::path get_temp_directory_path()
{
    auto root = get_root_directory_path();
    root /= "temp";
    return root;
}

std::filesystem::path get_backup_directory_path()
{
    auto root = get_root_directory_path();
    root /= "backup";
    return root;
}

std::filesystem::path get_crash_dump_directory_path()
{
    auto root = get_root_directory_path();
    root /= "crash_dump";
    return root;
}


}

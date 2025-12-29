
namespace fs
{

std::filesystem::path get_this_app_filepath()
{
#ifdef _WIN32
    std::vector<wchar_t> buff(10);
    DWORD copied = 0;
    do {
        buff.resize(buff.size() * 2);
        copied = GetModuleFileNameW(NULL, buff.data(), static_cast<DWORD>(buff.size()));
    }
    while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    return std::filesystem::path(buff.data());

#elif defined __linux__
#include <unistd.h>
#include <limits.h>	
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1)
        return std::filesystem::path(std::string(result, (count > 0) ? count : 0));
    return {};

#elif defined __APPLE__
#include <mach-o/dyld.h>
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) == 0)
        return std::filesystem::path(buffer.data());
    return {};
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

std::filesystem::path get_crash_dump_directory_path()
{
    auto root = get_root_directory_path();
    root /= "crash_dump";
    return root;
}


}

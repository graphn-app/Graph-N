
// TODO: make tests

namespace tools
{
namespace str
{

std::string trim(std::string string, std::string chars /*= " \r\n\t"*/)
{
    if (string.empty())
        return string;

    std::string::size_type _off = string.find_first_not_of(chars);
    if (_off != std::string::npos)
    {
        if (_off != 0)
            string.erase(0, _off);

        _off = string.find_last_not_of(chars);
        if (_off != std::string::npos && _off != string.size() - 1)
            string.erase(++_off);
    }
    else
        string.clear();

    return string;
}

std::string replace(std::string string, const std::string& find, const std::string& replace, size_t from_pos /*= 0*/)
{
    std::string::size_type pos = from_pos;
    while ((pos = string.find(find, pos)) != std::string::npos)
    {
        string.replace(pos, find.size(), replace);
        pos += replace.length();
    }
    return string;
}

std::string format(const char* format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    int size = _vscprintf(format, arglist);
    std::string str;
    str.resize(size);
    vsprintf(str.data(), format, arglist);
    va_end(arglist);
    return str;
}

std::list<std::string> split(const std::string& str, char separator, bool add_empty)
{
    size_t sep_count = std::count(str.begin(), str.end(), separator);

    std::list <std::string> out;
    std::istringstream stream(str.data());
    for (std::string l; std::getline(stream, l, separator); )
    {
        out.emplace_back(std::move(trim(std::move(l))));
    }

    if (add_empty && sep_count + 1 != out.size())
        out.push_back("");

    return out;
}

std::string to_lower_ansi(const std::string& string)
{
    std::string lower = string;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](const char c)
    {
        return cast::to_char(std::tolower(c));
    }
    );
    return lower;
}

size_t hash_string(const std::string& str)
{
    return std::hash<std::string>{}(str);
}

size_t hash_fnv1a(const void* data, size_t size)
{
    // FNV-1a constants for 64-bit
    constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325;
    constexpr uint64_t FNV_PRIME = 0x100000001b3;

    size_t hash = FNV_OFFSET_BASIS;
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i)
    {
        hash ^= ptr[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

size_t hash_fnv1a(const char* data, size_t size)
{
    return hash_fnv1a((void*)data, size);
}

size_t hash_fnv1a(size_t data)
{
    return hash_fnv1a((void*)&data, sizeof(data));
}

size_t hash_fnv1a(const std::string& data)
{
    return hash_fnv1a((void*)data.c_str(), data.size());
}

} // namespace str

namespace path
{

bool exists(const std::filesystem::path& path)
{
    try
    {
        return !path.empty() && std::filesystem::exists(path);
    }
    catch (...)
    {
    }
    return false;
}

std::optional<std::string> copy(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename)
{
    LogV(from, to, overwrite, rename);

    try
    {
        if (from.empty() || to.empty())
            return {};

        if (!std::filesystem::exists(from))
        {
            Log("Can't copy file from '" << from << "' - source not exists");
            return {};
        }

        const bool exists = std::filesystem::exists(to);
        if (exists && !overwrite && !rename)
        {
            Log("Can't copy file from '" << from << "' to '" << to << "' - dest file exists");
            return {};
        }

        std::string dest = to.string();
        if (exists && !overwrite && rename)
            dest = find_next_name(to.string());

        std::error_code err;
        std::filesystem::copy_file(from, dest, std::filesystem::copy_options::overwrite_existing, err);
        if (!std::filesystem::exists(dest))
        {
            LogError(ExcepCategory::filesystem, "Can't copy file '" << from << "' to '" << dest << "' error " << err.value() << ": " << err.message());
            return {};
        }

        Log("File copied from '" << from << "' to '" << dest << "'");
        return dest;
    }
    catch (...)
    {
        LogError(ExcepCategory::filesystem, "Copy exp: Can't move file from '" << from << "' to '" << to);
    }
    return {};
}

std::optional<std::string> move(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename)
{
    LogV(from, to, overwrite, rename);

    try
    {
        if (from.empty() || to.empty())
            return {};

        if (!std::filesystem::exists(from))
        {
            Log("Can't move file from '" << from << "' - source not exists");
            return {};
        }

        const bool exists = std::filesystem::exists(to);
        if (exists && !overwrite && !rename)
        {
            Log("Can't move file from '" << from << "' to '" << to << "' - dest file exists");
            return {};
        }

        std::string dest = to.string();
        if (exists && !overwrite && rename)
            dest = find_next_name(to.string());

        std::error_code err;
        std::filesystem::rename(from, dest, err);
        if (!std::filesystem::exists(dest))
        {
            LogError(ExcepCategory::filesystem, "Can't move file '" << from << "' to '" << dest << "' error " << err.value() << ": " << err.message());
            return {};
        }

        Log("File moved from '" << from << "' to '" << dest << "'");
        return dest;
    }
    catch (...)
    {
        LogError(ExcepCategory::filesystem, "Move exp: Can't move file from '" << from << "' to '" << to);
    }
    return {};
}

std::string find_next_name(std::filesystem::path path)
{
    std::string ext = path.extension().string();
    std::string path_no_ext = path.replace_extension().string();
    std::string next;
    for (size_t idx = 2; ; idx++)
    {
        next = path_no_ext + " (" + std::to_string(idx) + ")" + ext;
        if (!tools::path::exists(next))
            break;
    }
    return next;
}

bool make_folder(const std::filesystem::path& dirpath)
{
    Log(dirpath);

    try
    {
        if (dirpath.empty())
            return false;
        if (std::filesystem::exists(dirpath))
            return true;

        std::error_code err;
        std::filesystem::create_directories(dirpath, err);
        if (!std::filesystem::exists(dirpath))
        {
            LogError(ExcepCategory::filesystem, "Can't create dir '" << dirpath << "' error " << err.value() << ": " << err.message());
            return false;
        }

        Log("Created directory '" << dirpath << "'");
        return true;
    }
    catch (...)
    {
        LogError(ExcepCategory::filesystem, "Can't create directory " << dirpath);
    }
    return false;
}

bool make_file(const std::filesystem::path& filepath, bool overwrite, const std::string& data_to_write /*= ""*/)
{
    LogV(filepath, overwrite);

    if (filepath.empty())
        return false;

    if (tools::path::exists(filepath) && !overwrite)
    {
        Log("Can't create file '" << filepath << "' - file already exists");
        return false;
    }

    try
    {
        std::ofstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        file.open(filepath.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

        if (!data_to_write.empty())
            file << data_to_write;

        Log("Created file " << filepath);
        return true;
    }
    catch (const std::ios_base::failure& e)
    {
        LogError(ExcepCategory::filesystem, "Can't create/write file " << filepath << ", err: " << e.what() << ", code: " << e.code().value());
    }
    return false;
}

bool make_file_path(const std::filesystem::path& filepath, bool overwrite)
{
    auto dir = filepath;
    if (!make_folder(dir.remove_filename()))
        return false;
    return make_file(filepath, overwrite);
}

std::vector<char> read_file(const std::filesystem::path& filepath)
{
    Log(filepath);

    if (!tools::path::exists(filepath))
        return {};

    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        file.open(filepath, std::ios::binary | std::ios::ate);

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size))
            return buffer;
    }
    catch (const std::ios_base::failure& e)
    {
        LogError(ExcepCategory::filesystem, "Can't open/read file " << filepath << ", err: " << e.what() << ", code: " << e.code().value());
    }
    return {};
}

bool remove(const std::filesystem::path& filepath)
{
    Log(filepath);

    if (!tools::path::exists(filepath))
        return true;

    std::error_code err;
    std::filesystem::remove_all(filepath, err);
    if (tools::path::exists(filepath))
    {
        LogError(ExcepCategory::filesystem, "Can't remove file " << filepath << ", err " << err.value() << ", " << err.message());
        return false;
    }

    Log("Removed " << filepath);
    return true;
}

bool rename(const std::filesystem::path& from, const std::filesystem::path& to)
{
    LogV(from, to);

    if (from.empty() || to.empty())
        return false;

    if (!tools::path::exists(from))
    {
        Log("Can't rename file, source not exists");
        return true;
    }

    std::error_code err;
    std::filesystem::rename(from, to, err);
    if (tools::path::exists(from) || !tools::path::exists(to))
    {
        LogError(ExcepCategory::filesystem, "Can't rename, error " << err.value() << ": " << err.message());
        return false;
    }

    Log("Renamed from " << from << " to " << to);
    return true;
}

bool is_file(const std::filesystem::path& path)
{
    if (path.empty())
        return false;
    return !is_dir(path);
}

bool is_dir(const std::filesystem::path& path)
{
    if (path.empty())
        return false;
    return std::filesystem::is_directory(path);
}

bool is_empty(const std::filesystem::path& path)
{
    std::error_code err;
    return std::filesystem::is_empty(path, err);
}

std::string& add_backslash(std::string& path)
{
    if (path.empty() || path[path.size() - 1] != '\\')
        path += "\\";
    return path;
}

std::string& remove_backslash(std::string& path)
{
    if (path.empty())
        return path;
    while ('\\' == path[path.size() - 1])
    {
        path.erase(path.size() - 1);
    }
    return path;
}

std::list<std::filesystem::path> enumerate_directory(const std::string& path, bool recursively, bool include_dirs, bool include_files, bool relative_pathes, char slash /*= '\\'*/)
{
    std::list<std::filesystem::path> out;

    std::string root = tools::str::replace(path, "/", "\\", 0);
    add_backslash(root);

    const auto replace_slash = [slash](const std::string& path) -> std::string
    {
        if (slash != '\\')
            return tools::str::replace(path, "\\", std::string(1, slash));
        return path;
    };

    const auto relative_path = [&root, relative_pathes, &replace_slash](const std::string& path) -> std::string
    {
        if (!relative_pathes)
            return replace_slash(path);

        const auto pos = path.find(root);
        if (0 == pos)
            return replace_slash(path.substr(root.size()));
        return replace_slash(path);
    };

    if (recursively)
    {
        for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(root))
        {
            if (dir_entry.is_directory() && include_dirs)
                out.push_back(relative_path(dir_entry.path().string()));
            else if (!dir_entry.is_directory() && include_files)
                out.push_back(relative_path(dir_entry.path().string()));
        }
    }
    else
    {
        for (auto const& dir_entry : std::filesystem::directory_iterator(root))
        {
            if (dir_entry.is_directory() && include_dirs)
                out.push_back(relative_path(dir_entry.path().string()));
            else if (!dir_entry.is_directory() && include_files)
                out.push_back(relative_path(dir_entry.path().string()));
        }
    }
    return out;
}

}	// namespace path

}	//	namespace tools

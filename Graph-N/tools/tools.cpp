
namespace tools
{
namespace str
{

std::string& trim_this(std::string& str, const std::string& chars /*= " \r\n\t"*/)
{
    if (str.empty())
        return str;

    const auto first = str.find_first_not_of(chars);
    if (first == std::string::npos)
    {
        str.clear();
        return str;
    }

    const auto last = str.find_last_not_of(chars);
    const auto count = last - first + 1;
    if (first == 0 && count == str.size())
        return str;

    str = str.substr(first, count);
    return str;
}

std::string trim_their(const std::string& str, const std::string& chars /*= " \r\n\t"*/)
{
    if (str.empty())
        return {};

    const auto first = str.find_first_not_of(chars);
    if (first == std::string::npos)
        return {};

    const auto last = str.find_last_not_of(chars);
    const auto count = last - first + 1;
    if (first == 0 && count == str.size())
        return str;

    return str.substr(first, count);
}

std::string& replace_this(std::string& str, const std::string& find, const std::string& replace, size_t from_pos /*= 0*/)
{
    if (find.empty())
        return str;

    std::string::size_type pos = from_pos;
    while ((pos = str.find(find, pos)) != std::string::npos)
    {
        str.replace(pos, find.size(), replace);
        pos += replace.length();
    }
    return str;
}

std::string replace_their(const std::string& str, const std::string& find, const std::string& replace, size_t from_pos /*= 0*/)
{
    if (find.empty())
        return str;

    std::string copy = str;
    replace_this(copy, find, replace, from_pos);
    return copy;
}

std::vector<std::string> split(const std::string& str, char separator)
{
    std::vector<std::string> out;
    if (!str.empty())
        out.reserve(str.size() / 8);

    size_t start = 0;
    size_t end = str.find(separator);

    while (end != std::string::npos)
    {
        std::string token = str.substr(start, end - start);
        out.emplace_back(std::move(trim_this(token)));

        start = end + 1;
        end = str.find(separator, start);
    }

    std::string last_token = str.substr(start);
    out.emplace_back(std::move(trim_this(last_token)));

    return out;
}

std::string to_lower_ascii(const std::string& str)
{
    std::string lower;
    lower.reserve(str.size());
    std::transform(str.begin(), str.end(), std::back_inserter(lower),
        [](unsigned char c)
        {
            return cast::to_char(std::tolower(c));
        });
    return lower;
}

size_t hash_string(const std::string& str)
{
    return std::hash<std::string>{}(str);
}

uint64_t hash_fnv1a(const void* data, size_t size)
{
    // FNV-1a constants for 64-bit
    constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325;
    constexpr uint64_t FNV_PRIME = 0x100000001b3;

    uint64_t hash = FNV_OFFSET_BASIS;
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i)
    {
        hash ^= ptr[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

uint64_t hash_fnv1a(const char* data, size_t size)
{
    return hash_fnv1a((const void*)data, size);
}

uint64_t hash_fnv1a(size_t data)
{
    return hash_fnv1a((const void*)&data, sizeof(data));
}

uint64_t hash_fnv1a(const std::string& data)
{
    return hash_fnv1a((const void*)data.c_str(), data.size());
}

} // namespace str

namespace path
{

// --- HELPERS FOR UTF-8 ---
inline std::filesystem::path from_utf8(const std::string& u8str)
{
    return std::filesystem::u8path(u8str);
}

inline std::string to_utf8(const std::filesystem::path& p)
{
    return p.u8string();
}

// -------------------------

std::filesystem::path find_next_name(const std::filesystem::path& path); // forward decl

bool exists(const std::filesystem::path& path)
{
    std::error_code ec;
    return !path.empty() && std::filesystem::exists(path, ec);
}

std::optional<std::filesystem::path> copy(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename)
{
    LogV(from, to, overwrite, rename);

    if (from.empty() || to.empty())
        return {};

    std::error_code ec;
    if (!std::filesystem::exists(from, ec))
    {
        Log("Can't copy file from '" << from << "' - source not exists, err " << ec.value() << ": " << ec.message());
        return std::nullopt;
    }

    const bool exists = std::filesystem::exists(to);
    if (exists && !overwrite && !rename)
    {
        Log("Can't copy file from '" << from << "' to '" << to << "' - dest file exists");
        return std::nullopt;
    }

    std::optional<std::filesystem::path> dest;
    auto options = std::filesystem::copy_options::none;

    if (exists && !overwrite && rename)
    {
        dest = std::move(find_next_name(to));
    }
    else
    {
        dest = to;
        options = std::filesystem::copy_options::overwrite_existing;
    }

    if (!std::filesystem::copy_file(from, dest.value(), options, ec))
    {
        LogError(ExcepCategory::filesystem, "Can't copy file '" << from << "' to '" << dest.value() << "' error " << ec.value() << ": " << ec.message());
        return std::nullopt;
    }

    Log("File copied from '" << from << "' to '" << dest.value() << "'");
    return dest;
}

std::optional<std::filesystem::path> move(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename)
{
    LogV(from, to, overwrite, rename);

    if (from.empty() || to.empty())
        return {};

    std::error_code ec;
    if (!std::filesystem::exists(from, ec))
    {
        Log("Can't move file from '" << from << "' - source not exists, err " << ec.value() << ": " << ec.message());
        return std::nullopt;
    }

    const bool exists = std::filesystem::exists(to);
    if (exists && !overwrite && !rename)
    {
        Log("Can't move file from '" << from << "' to '" << to << "' - dest file exists");
        return std::nullopt;
    }

    std::optional<std::filesystem::path> dest;
    if (exists && !overwrite && rename)
    {
        dest = std::move(find_next_name(to));
    }
    else
    {
        dest = to;
    }

    std::filesystem::rename(from, dest.value(), ec);
    if (ec)
    {
        if (!tools::path::copy(from, dest.value(), true, false))
        {
            LogError(ExcepCategory::filesystem, "Can't move file '" << from << "' to '" << dest.value() << "' error " << ec.value() << ": " << ec.message());
            return std::nullopt;
        }
        tools::path::remove(from);
    }

    Log("File moved from '" << from << "' to '" << dest.value() << "'");
    return dest;
}

std::filesystem::path find_next_name(const std::filesystem::path& path)
{
    const auto ext = path.extension().u8string();
    const auto parent = path.parent_path();
    const auto stem = path.stem().u8string();

    size_t idx = 2;
    std::filesystem::path next_path;
    std::error_code ec;

    do {
        const std::string new_name = stem + " (" + std::to_string(idx++) + ")" + ext;
        next_path = parent / std::filesystem::u8path(new_name);
    }
    while (std::filesystem::exists(next_path, ec));

    return next_path;
}

bool make_folder(const std::filesystem::path& dirpath)
{
    Log(dirpath);

    if (dirpath.empty())
        return false;

    std::error_code ec;
    std::filesystem::create_directories(dirpath, ec);
    if (ec)
    {
        LogError(ExcepCategory::filesystem, "Can't create dir '" << dirpath << "' error " << ec.value() << ": " << ec.message());
        return false;
    }

    Log("Created directory '" << dirpath << "'");
    return true;
}

bool make_file(const std::filesystem::path& filepath, bool overwrite, const std::string& data_to_write /*= ""*/)
{
    LogV(filepath, overwrite, data_to_write.size());

    if (filepath.empty())
        return false;

    std::error_code ec;
    if (!overwrite && std::filesystem::exists(filepath, ec))
    {
        Log("Can't create file '" << filepath << "' - file already exists");
        return false;
    }

    std::ofstream file(filepath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open())
    {
        LogError(ExcepCategory::filesystem, "Can't create file " << filepath);
        return false;
    }

    if (!data_to_write.empty())
    {
        file.write(data_to_write.data(), data_to_write.size());
        if (file.fail())
        {
            LogError(ExcepCategory::filesystem, "Write failed " << filepath);
            return false;
        }
    }

    Log("Created file " << filepath << " and written " << data_to_write.size() << " bytes");
    return true;
}

bool make_file_path(const std::filesystem::path& filepath, bool overwrite, const std::string& data_to_write /*= ""*/)
{
    if (!make_folder(filepath.parent_path()))
        return false;
    return make_file(filepath, overwrite, data_to_write);
}

std::vector<char> read_file(const std::filesystem::path& filepath)
{
    Log(filepath);

    std::error_code ec;
    const auto fsize = std::filesystem::file_size(filepath, ec);
    if (ec)
    {
        LogError(ExcepCategory::filesystem, "Can't read file " << filepath << ", error " << ec.value() << ": " << ec.message());
        return {};
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        LogError(ExcepCategory::filesystem, "Can't open file " << filepath);
        return {};
    }

    try
    {
        std::vector<char> buffer;
        buffer.resize(cast::to_szt(fsize));

        file.read(buffer.data(), fsize);
        if (!file)
        {
            buffer.resize(file.gcount());
        }
        return buffer;
    }
    catch (...)
    {
        LogError(ExcepCategory::filesystem, "Can't read file " << filepath);
        return {};
    }
}

bool remove(const std::filesystem::path& filepath)
{
    Log(filepath);

    std::error_code ec;
    std::filesystem::remove_all(filepath, ec);
    if (ec)
    {
        LogError(ExcepCategory::filesystem, "Can't remove file " << filepath << ", err " << ec.value() << ", " << ec.message());
        return false;
    }

    Log("Removed " << filepath);
    return true;
}

bool rename(const std::filesystem::path& from, const std::filesystem::path& to)
{
    LogV(from, to);

    return tools::path::move(from, to, false, false).has_value();
}

bool is_file(const std::filesystem::path& path)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return false;
    return std::filesystem::is_regular_file(path, ec);
}

bool is_dir(const std::filesystem::path& path)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return false;
    return std::filesystem::is_directory(path, ec);
}

bool is_empty(const std::filesystem::path& path)
{
    std::error_code ec;
    return std::filesystem::is_empty(path, ec);
}

std::vector<std::filesystem::path> enumerate_directory(
    const std::filesystem::path& path,
    bool recursively, bool include_dirs, bool include_files, bool relative_pathes)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_directory(path, ec))
        return {};

    std::vector<std::filesystem::path> out;
    out.reserve(100);

    const auto process_entry = [&](const std::filesystem::directory_entry& entry)
    {
        const bool is_dir = entry.is_directory();
        if ((is_dir && !include_dirs) || (!is_dir && !include_files))
            return;

        // relative vs absolute
        std::filesystem::path current_path;
        if (relative_pathes)
        {
            current_path = entry.path().lexically_relative(path);
        }
        else
        {
            current_path = entry.path();
        }
        out.emplace_back(std::move(current_path));
    };

    try
    {
        const auto opts = std::filesystem::directory_options::skip_permission_denied;
        if (recursively)
        {
            for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(path, opts))
            {
                process_entry(dir_entry);
            }
        }
        else
        {
            for (auto const& dir_entry : std::filesystem::directory_iterator(path, opts))
            {
                process_entry(dir_entry);
            }
        }
    }
    catch (...)
    {
        LogError(ExcepCategory::filesystem, "Error enumerating directory " << path);
    }

    return out;
}

std::vector<std::string> enumerate_directory_set_slash(const std::filesystem::path& path,
    bool recursively, bool include_dirs, bool include_files, bool relative_pathes, char slash)
{
    const auto paths = enumerate_directory(path, recursively, include_dirs, include_files, relative_pathes);
    std::vector<std::string> out;
    out.reserve(paths.size());

    for (auto& p : paths)
    {
        auto str = p.generic_u8string();
        if (slash != '/')
            std::replace(str.begin(), str.end(), '/', slash);
        out.emplace_back(std::move(str));
    }
    return out;
}

}	// namespace path

}	//	namespace tools

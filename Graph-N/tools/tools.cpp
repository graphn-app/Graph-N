
#ifdef _WIN32
#include "main/main_win32.h"
#endif

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
        trim_this(token);
        if (!token.empty())
            out.push_back(std::move(token));

        start = end + 1;
        end = str.find(separator, start);
    }

    std::string last_token = str.substr(start);
    trim_this(last_token);
    if (!last_token.empty())
        out.push_back(std::move(last_token));

    return out;
}

template<typename CONT>
static std::string merge_cont(const CONT& items, char separator)
{
    std::string out;
    out.reserve(items.size() * 10);
    bool first = true;
    for (auto& item : items)
    {
        if (first)
        {
            out.append(item);
            first = false;
        }
        else
        {
            out.append(1, separator).append(item);
        }
    }
    return out;
}

std::string merge(const std::vector<std::string>& items, char separator)
{
    return merge_cont(items, separator);
}

std::string merge(const std::set<std::string>& items, char separator)
{
    return merge_cont(items, separator);
}

std::string to_lower_ascii(const std::string& str)
{
    std::string lower;
    lower.reserve(str.size());
    std::transform(str.begin(), str.end(), std::back_inserter(lower),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
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

uint32_t crc32(const std::string& data)
{
    const auto generate_table = []()
    {
        std::array<uint32_t, 256> table;
        uint32_t polynomial = 0xEDB88320;

        for (uint32_t i = 0; i < 256; ++i)
        {
            uint32_t crc = i;
            for (int32_t j = 0; j < 8; ++j)
            {
                if (crc & 1)
                    crc = (crc >> 1) ^ polynomial;
                else
                    crc >>= 1;                
            }
            table[i] = crc;
        }
        return table;
    };
    static std::array<uint32_t, 256> table = generate_table();

    const char* pdata = data.c_str();
    const size_t length = data.size();
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; ++i)
    {
        const uint8_t byte = static_cast<uint8_t>(pdata[i]);
        const uint32_t lookup_index = (crc ^ byte) & 0xFF;
        crc = (crc >> 8) ^ table[lookup_index];
    }

    return ~crc;
}

std::string generate_uuid_v4()
{
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    static thread_local std::uniform_int_distribution<uint64_t> dis;

    uint64_t part1 = dis(gen); 
    uint64_t part2 = dis(gen);

    // version
    part1 = (part1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    // variant
    part2 = (part2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    // UUID : 8-4-4-4-12 
    ss << std::setw(8) << (part1 >> 32) << "-";
    ss << std::setw(4) << ((part1 >> 16) & 0xFFFF) << "-";
    ss << std::setw(4) << (part1 & 0xFFFF) << "-";
    ss << std::setw(4) << (part2 >> 48) << "-";
    ss << std::setw(12) << (part2 & 0xFFFFFFFFFFFFULL);

    return ss.str();
}

std::vector<byte> base64_decode(const std::string& base64)
{
    constexpr const std::string_view base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<byte> out;
    out.reserve(base64.size() * 3 / 4);

    std::vector<int32_t> T(256, -1);
    for (int i = 0; i < 64; i++)
    {
        T[base64_chars[i]] = i;
    }

    int val = 0, valb = -8;
    for (byte c : base64)
    {
        if (T[c] == -1)
            break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::vector<byte> base64_decode(const char* base64, size_t len)
{
    constexpr const std::string_view base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<byte> out;
    out.reserve(len * 3 / 4);

    std::vector<int32_t> T(256, -1);
    for (int i = 0; i < 64; i++)
    {
        T[base64_chars[i]] = i;
    }

    int val = 0, valb = -8;
    for (size_t idx = 0; idx < len; idx++)
    {
        const char c = base64[idx];
        if (T[c] == -1)
            break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

static constexpr uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

std::string sha256(const std::vector<byte>& src)
{
    uint32_t h[8] =
    {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    constexpr const uint32_t k[64] =
    {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    std::vector<byte> buffer;
    buffer.reserve(src.size() + 10);
    buffer.assign(src.begin(), src.end());

    // Padding
    buffer.push_back(0x80);
    while ((buffer.size() + 8) % 64 != 0)
    {
        buffer.push_back(0x00);
    }

    const size_t bit_len = src.size() * 8;
    for (int i = 7; i >= 0; --i)
    {
        buffer.push_back((bit_len >> (i * 8)) & 0xff);
    }

    for (size_t i = 0; i < buffer.size(); i += 64)
    {
        uint32_t w[64], a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], h_val = h[7];

        for (int j = 0; j < 64; ++j)
        {
            if (j < 16)
            {
                w[j] = (buffer[i + j * 4] << 24) | (buffer[i + j * 4 + 1] << 16) | (buffer[i + j * 4 + 2] << 8) | buffer[i + j * 4 + 3];
            }
            else
            {
                uint32_t s0 = rotr(w[j - 15], 7) ^ rotr(w[j - 15], 18) ^ (w[j - 15] >> 3);
                uint32_t s1 = rotr(w[j - 2], 17) ^ rotr(w[j - 2], 19) ^ (w[j - 2] >> 10);
                w[j] = w[j - 16] + s0 + w[j - 7] + s1;
            }
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = h_val + S1 + ch + k[j] + w[j];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            h_val = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += h_val;
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i)
    {
        ss << std::setw(8) << h[i];
    }
    return ss.str();
}

#ifdef _WIN32
std::wstring utf8_to_wide(const std::string& utf8_str)
{
    if (utf8_str.empty())
        return {};

    const auto size_needed = ::MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), cast::to_i32(utf8_str.size()), nullptr, 0);
    std::wstring wide_str(size_needed, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), cast::to_i32(utf8_str.size()), wide_str.data(), size_needed);
    return wide_str;
}
#endif // #ifdef _WIN32

std::string cleanup_url(const std::string& value) 
{
    std::string escaped;
    escaped.reserve(value.length() * 3); 

    for (const char c : value) 
    {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') 
        {
            escaped.push_back(c);
        } 
        else 
        {
            escaped += fmt::format("%{:02X}", static_cast<unsigned char>(c));
        }
    }

    return escaped;
}

} // namespace str

namespace path
{

// --- HELPERS FOR UTF-8 ---
static inline std::filesystem::path from_utf8(const std::string& u8str)
{
    return std::filesystem::u8path(u8str);
}

static inline std::string to_utf8(const std::filesystem::path& p)
{
    return p.u8string();
}

// -------------------------

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

bool exists(const std::filesystem::path& path)
{
    std::error_code ec;
    return !path.empty() && std::filesystem::exists(path, ec);
}

std::optional<std::filesystem::path> copy(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename)
{
    LogVS(from, to, overwrite, rename);

    if (from.empty() || to.empty())
        return {};

    std::error_code ec;
    if (!std::filesystem::exists(from, ec))
    {
        LogS("Can't copy file from '" << from << "' - source not exists, err " << ec.value() << ": " << ec.message());
        return std::nullopt;
    }

    const bool exists = std::filesystem::exists(to);
    if (exists && !overwrite && !rename)
    {
        LogS("Can't copy file from '" << from << "' to '" << to << "' - dest file exists");
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
        LogErrorS(ExcepCategory::FileSystem, "Can't copy file '" << from << "' to '" << dest.value() << "' error " << ec.value() << ": " << ec.message());
        return std::nullopt;
    }

    LogS("File copied from '" << from << "' to '" << dest.value() << "'");
    return dest;
}

std::optional<std::filesystem::path> move(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename)
{
    LogVS(from, to, overwrite, rename);

    if (from.empty() || to.empty())
        return {};

    std::error_code ec;
    if (!std::filesystem::exists(from, ec))
    {
        LogS("Can't move file from '" << from << "' - source not exists, err " << ec.value() << ": " << ec.message());
        return std::nullopt;
    }

    const bool exists = std::filesystem::exists(to);
    if (exists && !overwrite && !rename)
    {
        LogS("Can't move file from '" << from << "' to '" << to << "' - dest file exists");
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
            LogErrorS(ExcepCategory::FileSystem, "Can't move file '" << from << "' to '" << dest.value() << "' error " << ec.value() << ": " << ec.message());
            return std::nullopt;
        }
        tools::path::remove(from);
    }

    LogS("File moved from '" << from << "' to '" << dest.value() << "'");
    return dest;
}

bool make_folder(const std::filesystem::path& dirpath)
{
    LogS(dirpath);

    if (dirpath.empty())
        return false;

    std::error_code ec;
    std::filesystem::create_directories(dirpath, ec);
    if (ec)
    {
        LogErrorS(ExcepCategory::FileSystem, "Can't create dir '" << dirpath << "' error " << ec.value() << ": " << ec.message());
        return false;
    }

    LogS("Created directory '" << dirpath << "'");
    return true;
}

bool make_file(const std::filesystem::path& filepath, bool overwrite, const char* data_to_write, size_t data_len)
{
    LogVS(filepath, overwrite, data_len);

    if (filepath.empty())
        return false;

    std::error_code ec;
    if (!overwrite && std::filesystem::exists(filepath, ec))
    {
        LogS("Can't create file '" << filepath << "' - file already exists");
        return false;
    }

    std::ofstream file(filepath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open())
    {
        LogErrorS(ExcepCategory::FileSystem, "Can't create file " << filepath);
        return false;
    }

    if (data_len > 0)
    {
        file.write(data_to_write, data_len);
        if (file.fail())
        {
            LogErrorS(ExcepCategory::FileSystem, "Write failed " << filepath);
            return false;
        }
    }

    LogS("Created file " << filepath << " and written " << data_len << " bytes");
    return true;
}

bool make_file(const std::filesystem::path& filepath, bool overwrite, const std::vector<byte>& data_to_write)
{
    return make_file(filepath, overwrite, (const char*)data_to_write.data(), data_to_write.size());
}

bool make_file(const std::filesystem::path& filepath, bool overwrite, const std::string& data_to_write)
{
    return make_file(filepath, overwrite, data_to_write.data(), data_to_write.size());
}

bool make_file_path(const std::filesystem::path& filepath, bool overwrite, const std::vector<byte>& data_to_write)
{
    if (!make_folder(filepath.parent_path()))
        return false;
    return make_file(filepath, overwrite, data_to_write);
}

bool make_file_path(const std::filesystem::path& filepath, bool overwrite, const std::string& data_to_write)
{
    if (!make_folder(filepath.parent_path()))
        return false;
    return make_file(filepath, overwrite, data_to_write);
}

std::vector<char> read_file(const std::filesystem::path& filepath, bool add_tail)
{
    LogS(filepath);

    std::error_code ec;
    const auto fsize = std::filesystem::file_size(filepath, ec);
    if (ec)
    {
        LogErrorS(ExcepCategory::FileSystem, "Can't read file " << filepath << ", error " << ec.value() << ": " << ec.message());
        return {};
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        LogErrorS(ExcepCategory::FileSystem, "Can't open file " << filepath);
        return {};
    }

    try
    {
        std::vector<char> buffer;
        buffer.resize(cast::to_szt(fsize) + (add_tail ? 1 : 0));

        file.read(buffer.data(), fsize);
        if (!file)
        {
            buffer.resize(file.gcount());
        }
        return buffer;
    }
    catch (...)
    {
        LogErrorS(ExcepCategory::FileSystem, "Can't read file " << filepath);
        return {};
    }
}

bool remove(const std::filesystem::path& filepath)
{
    LogS(filepath);

    std::error_code ec;
    std::filesystem::remove_all(filepath, ec);
    if (ec)
    {
        LogErrorS(ExcepCategory::FileSystem, "Can't remove file " << filepath << ", err " << ec.value() << ", " << ec.message());
        return false;
    }

    LogS("Removed " << filepath);
    return true;
}

bool rename(const std::filesystem::path& from, const std::filesystem::path& to)
{
    LogVS(from, to);

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

std::string get_extension(const std::filesystem::path& path)
{
    if (path.empty())
        return {};
    return path.extension().u8string();
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
        out.push_back(std::move(current_path));
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
        LogErrorS(ExcepCategory::FileSystem, "Error enumerating directory " << path);
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
        out.push_back(std::move(str));
    }
    return out;
}

#ifdef _WIN32
struct FdData
{
    HWND hwnd = nullptr;
    bool read = false;
    bool sel_directory = false;
    bool allow_multiselect = false;
    std::list<std::pair<std::wstring, std::wstring>> filters;
    std::filesystem::path init_file;
    FdSelectResultSg callback_sg = nullptr;
    FdSelectResultMt callback_mt = nullptr;
    std::list<std::filesystem::path> results;
};

static bool select_files(HWND hwnd, bool read, bool sel_directory, bool allow_multiselect, 
    const std::list<std::pair<std::wstring, std::wstring>>& filters, const std::filesystem::path& init_file,
    std::list<std::filesystem::path>& results)
{
    results.clear();
    bool result = false;
    HRESULT hr = S_OK;
    CComPtr<IFileDialog> pFileDlg;
    if (read)
        hr = pFileDlg.CoCreateInstance(__uuidof(FileOpenDialog));
    else
        hr = pFileDlg.CoCreateInstance(__uuidof(FileSaveDialog));
    if (SUCCEEDED(hr))
    {
        DWORD dwOptions = 0;
        pFileDlg->GetOptions(&dwOptions);
        if (!read)
        {
            dwOptions |= FOS_OVERWRITEPROMPT | FOS_DEFAULTNOMINIMODE;
            dwOptions &= ~FOS_NOREADONLYRETURN;
        }
        else
        {
            dwOptions |= FOS_FORCEPREVIEWPANEON;
        }
        if (allow_multiselect)
            dwOptions |= FOS_ALLOWMULTISELECT;
        if (sel_directory)
            dwOptions |= FOS_PICKFOLDERS;
        pFileDlg->SetOptions(dwOptions | FOS_FORCEFILESYSTEM);
        pFileDlg->SetTitle(APP_CAPTION_W);
        if (!read && !sel_directory && !init_file.empty())
            pFileDlg->SetFileName(init_file.wstring().c_str());

        if (!filters.empty())
        {
            std::vector<COMDLG_FILTERSPEC> types;
            for (auto& filter : filters)
            {
                types.push_back({ filter.first.c_str(), filter.second.c_str() });
            }
            pFileDlg->SetFileTypes(cast::to_u32(types.size()), types.data());
        }

        hr = pFileDlg->Show(hwnd);
        if (SUCCEEDED(hr))
        {
            CComPtr<IShellItemArray> pItemArray;
            if (read)
            {
                CComPtr<IFileOpenDialog> dlg;
                hr = pFileDlg->QueryInterface(&dlg);
                if (SUCCEEDED(hr))
                {
                    hr = dlg->GetResults(&pItemArray);
                    if (SUCCEEDED(hr))
                    {
                        DWORD dwNumItems = 0;
                        hr = pItemArray->GetCount(&dwNumItems);
                        if (SUCCEEDED(hr))
                        {
                            for (DWORD i = 0; i < dwNumItems; ++i)
                            {
                                CComPtr<IShellItem> pItem;
                                hr = pItemArray->GetItemAt(i, &pItem);
                                if (SUCCEEDED(hr))
                                {
                                    PWSTR pszFilePath = nullptr;
                                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                                    if (SUCCEEDED(hr))
                                    {
                                        results.push_back(pszFilePath);
                                        result = true;
                                            
                                        CoTaskMemFree(pszFilePath);
                                    }
                                    else
                                    {
                                        LogWarnS("GetDisplayName failed: " << LogHRES(hr));
                                    }
                                }
                                else
                                {
                                    LogWarnS("pItemArray->GetItemAt failed: " << LogHRES(hr));
                                }
                            }
                        }
                        else
                        {
                            LogWarnS("pItemArray->GetCount failed: " << LogHRES(hr));
                        }
                    }
                    else
                    {
                        LogWarnS("GetResults failed: " << LogHRES(hr));
                    }
                }
                else
                {
                    LogWarnS("QueryInterface to IFileOpenDialog failed: " << LogHRES(hr));
                }
            }
            else
            {
                CComPtr<IShellItem> pItem;
                hr = pFileDlg->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath = nullptr;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                    if (SUCCEEDED(hr))
                    {
                        result = true;
                        results.push_back(pszFilePath);                        
                        CoTaskMemFree(pszFilePath);
                    }
                    else
                    {
                        LogWarnS("GetDisplayName failed: " << LogHRES(hr));
                    }
                }
                else
                {
                    LogWarnS("GetResult failed: " << LogHRES(hr));
                }
            }
        }
        else
        {
            LogWarnS("pFileOpen->Show filed: " << LogHRES(hr));
        }
    }
    else
    {
        LogWarnS("CoCreateInstance failed: " << LogHRES(hr));
    }

    return result;
}

void select_files_result_handler(WPARAM, LPARAM l)
{
    std::unique_ptr<FdData> data((FdData*)l);
    if (data->callback_mt != nullptr)
        data->callback_mt(!data->results.empty(), std::move(data->results));
    else if (data->callback_sg != nullptr)
    {
        if (!data->results.empty())
            data->callback_sg(true, std::move(*data->results.begin()));
        else
            data->callback_sg(false, {});
    }
}

void select_files_th(std::unique_ptr<FdData> data)
{    
    const auto res = select_files(data->hwnd, data->read, data->sel_directory, data->allow_multiselect, data->filters, data->init_file, data->results);
    if (!res)
        data->results.clear();
    ((platform::MainRoutine_Win32*)g::pMain)->set_user_msg_callback(select_files_result_handler);
    auto h = data->hwnd;
    ::PostMessage(h, USER_MESSAGE, 0, (LPARAM)data.release());
}

#endif // #ifdef _WIN32

void select_file_open(std::list<std::pair<std::wstring, std::wstring>>&& filters, FdSelectResultSg& callback)
{
    auto data = std::make_unique<FdData>();
    data->hwnd = ((platform::MainRoutine_Win32*)g::pMain)->get_hwnd();
    data->read = true;
    data->sel_directory = false;
    data->allow_multiselect = false;
    data->filters = std::move(filters);
    data->callback_sg = callback;
    std::thread(select_files_th, std::move(data)).detach();
}

void select_files_open(std::list<std::pair<std::wstring, std::wstring>>&& filters, FdSelectResultMt& callback)
{
    auto data = std::make_unique<FdData>();
    data->hwnd = ((platform::MainRoutine_Win32*)g::pMain)->get_hwnd();
    data->read = true;
    data->sel_directory = false;
    data->allow_multiselect = true;
    data->filters = std::move(filters);
    data->callback_mt = callback;
    std::thread(select_files_th, std::move(data)).detach();
}

void select_file_save(std::list<std::pair<std::wstring, std::wstring>>&& filters, std::filesystem::path init_file, FdSelectResultSg& callback)
{
    auto data = std::make_unique<FdData>();
    data->hwnd = ((platform::MainRoutine_Win32*)g::pMain)->get_hwnd();
    data->read = false;
    data->sel_directory = false;
    data->allow_multiselect = false;
    data->filters = std::move(filters);
    data->init_file = std::move(init_file);
    data->callback_sg = callback;
    std::thread(select_files_th, std::move(data)).detach();
}

void select_dir(FdSelectResultSg& callback)
{
    auto data = std::make_unique<FdData>();
    data->hwnd = ((platform::MainRoutine_Win32*)g::pMain)->get_hwnd();
    data->read = true;
    data->sel_directory = true;
    data->allow_multiselect = false;
    data->callback_sg = callback;
    std::thread(select_files_th, std::move(data)).detach();
}

void select_dirs(FdSelectResultMt& callback)
{
    auto data = std::make_unique<FdData>();
    data->hwnd = ((platform::MainRoutine_Win32*)g::pMain)->get_hwnd();
    data->read = true;
    data->sel_directory = true;
    data->allow_multiselect = true;
    data->callback_mt = callback;
    std::thread(select_files_th, std::move(data)).detach();
}

#ifdef _WIN32
std::wstring build_commandline_arg(const std::wstring& arg)
{
    if (!arg.empty() && arg.find_first_of(L" \t\"") == std::wstring::npos)
        return arg;

    std::wstring result;
    result.reserve(arg.size() * 2 + 2);
    result.push_back(L'\"');

    for (size_t i = 0; i < arg.size(); ++i)
    {
        size_t backslashes = 0;

        while (i < arg.size() && arg[i] == L'\\')
        {
            backslashes++;
            i++;
        }

        if (i == arg.size())
        {
            result.append(backslashes * 2, L'\\');
        }
        else if (arg[i] == L'\"')
        {
            result.append(backslashes * 2 + 1, L'\\');
            result.push_back(L'\"');
        }
        else
        {
            result.append(backslashes, L'\\');
            result.push_back(arg[i]);
        }
    }

    result.push_back(L'\"');
    return result;
}

std::string build_commandline_arg(const std::string& arg)
{
    if (!arg.empty() && arg.find_first_of(" \t\"") == std::string::npos)
        return arg;

    std::string result;
    result.reserve(arg.size() * 2 + 2);
    result.push_back('\"');

    for (size_t i = 0; i < arg.size(); ++i)
    {
        size_t backslashes = 0;

        while (i < arg.size() && arg[i] == '\\')
        {
            backslashes++;
            i++;
        }

        if (i == arg.size())
        {
            result.append(backslashes * 2, '\\');
        }
        else if (arg[i] == '\"')
        {
            result.append(backslashes * 2 + 1, '\\');
            result.push_back('\"');
        }
        else
        {
            result.append(backslashes, '\\');
            result.push_back(arg[i]);
        }
    }

    result.push_back('\"');
    return result;
}
#endif // #ifdef _WIN32

void open_file(const std::filesystem::path& path)
{
#ifdef _WIN32
    const auto res = cast::from_ptr<int32_t>(ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    if (res <= 32)
    {
        LogWarnS("Can't run file '" << path << "', error " << LogERR);
    }
    else
    {
        LogS("File run " << path);
    }

#endif // #ifdef _WIN32
}

std::string& clear_filename(std::string& filename)
{
    tools::str::trim_this(filename, " .,\r\n\t");
    if (filename.empty())
    {
        filename = "_";
        return filename;
    }

    tools::str::replace_this(filename, "/", "_");
    tools::str::replace_this(filename, "\\", "_");
    tools::str::replace_this(filename, ":", "_");
    tools::str::replace_this(filename, "*", "_");
    tools::str::replace_this(filename, "?", "_");
    tools::str::replace_this(filename, "\"", "_");
    tools::str::replace_this(filename, "<", "_");
    tools::str::replace_this(filename, ">", "_");
    tools::str::replace_this(filename, "|", "_");

    auto low = tools::str::to_lower_ascii(filename);
    if ("con" == low || "prn" == low || "aux" == low || "nul" == low || "com1" == low || "com2" == low || "com3" == low ||
        "com4" == low || "com5" == low || "com6" == low || "com7" == low || "com8" == low || "com9" == low || "lpt1" == low ||
        "lpt2" == low || "lpt3" == low || "lpt4" == low || "lpt5" == low || "lpt6" == low || "lpt7" == low || "lpt8" == low ||
        "lpt9" == low)
    {
        filename += "_";
    }

    return filename;
}

}	// namespace path

namespace api
{
std::vector<std::string> scan_for_disks()
{
    std::vector<std::string> ret;
#ifdef _WIN32
    char buffer[512];
    const DWORD len = ::GetLogicalDriveStringsA(sizeof(buffer), buffer);
    if (0 == len)
    {
        LogErrorS(ExcepCategory::API, "GetLogicalDriveStringsA failed, err " << LogERR);
        return ret;
    }

    const char* drive = buffer;
    while (*drive)
    {
        // Filter: Only fixed drives (HDD/SSD). Ignore USB/Network to avoid lag.
        if (DRIVE_FIXED == ::GetDriveTypeA(drive))
        {
            ret.push_back(drive);
        }
        drive += strlen(drive) + 1;
    }
#elif defined __linux__
#error Not implemented
#elif defined __APPLE__
#error Not implemented
#endif

    return ret;
}

} // namespace api

namespace img
{
bool icomparer(char l, char r)
{
    return std::tolower(static_cast<unsigned char>(l)) == std::tolower(static_cast<unsigned char>(r));
}

static std::string cleanup_json(const std::vector<char>& buffer, size_t offset)
{
    size_t start = offset;
    while (start < buffer.size() && buffer[start] != '{')
        start++;

    int32_t depth = 0;
    bool in_string = false;
    bool is_escape = false;

    for (size_t i = start; i < buffer.size(); i++) 
    {
        const char c = buffer[i];

        // If the previous character was a backslash, skip current
        if (is_escape) 
        {
            is_escape = false;
            continue;
        }

        // Check for escape character
        if (c == '\\') 
        {
            is_escape = true;
            continue;
        }

        // Toggle string state on unescaped quotes
        if (c == '"') 
        {
            in_string = !in_string;
            continue;
        }

        // Count braces only if we are not inside a string literal
        if (!in_string) 
        {
            if (c == '{') depth++;
            else if (c == '}') depth--;
            if (depth < 0) return {};

            // When depth reaches 0, we found the exact end of the JSON object
            if (0 == depth) 
            {
                return std::string(buffer.begin() + start, buffer.begin() + i + 1);
            }
        }
    }

    return {};
}

static uint32_t read_be32(const unsigned char* data) 
{
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

static size_t extract_png_tEXt(const std::vector<char>& file_data, bool search_api) 
{
    if (file_data.size() < 33)
        return std::string::npos;

    // Check PNG magic signature
    const unsigned char png_sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    if (memcmp(file_data.data(), png_sig, 8) != 0)
        return std::string::npos;

    size_t offset = 8;
    while (offset + 8 <= file_data.size()) 
    {
        const uint32_t length = read_be32(reinterpret_cast<const unsigned char*>(file_data.data()) + offset);
        const std::string chunk_type(reinterpret_cast<const char*>(file_data.data() + offset + 4), 4);

        offset += 8; // Move to chunk data
        if (offset + length + 4 > file_data.size())
            break;

        if (chunk_type == "tEXt" || chunk_type == "iTXt" || chunk_type == "comf") 
        {
            const std::string_view json_id = (search_api ? "prompt" : "workflow");
            const auto from = file_data.data() + offset;
            const auto to = file_data.data() + offset + length;

            auto it = std::search(from, to, json_id.begin(), json_id.end(), icomparer);
            if (it != to)
                return offset + std::distance(from, it);
        }

        // Jump to next chunk (+4 to skip CRC)
        offset += length + 4; 
    }

    return std::string::npos;
}

std::string extract_workflow_from_png(const std::filesystem::path& webp_path)
{
    const auto file_data = tools::path::read_file(webp_path, false);
    if (file_data.empty())
        return {};

    auto pos = extract_png_tEXt(file_data, false);
    if (std::string::npos == pos)
    {
        pos = extract_png_tEXt(file_data, true);
        if (std::string::npos == pos)
            return {};
    }

    return cleanup_json(file_data, pos);
}

static uint32_t read_le32(const unsigned char* data) 
{
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

static size_t extract_webp_exif(const std::vector<char>& file_data, bool search_api) 
{
    if (file_data.size() < 12)
        return std::string::npos;

    // Check RIFF header and WEBP signature
    if (memcmp(file_data.data(), "RIFF", 4) != 0 || memcmp(file_data.data() + 8, "WEBP", 4) != 0) 
        return std::string::npos;

    size_t offset = 12;
    while (offset + 8 <= file_data.size()) 
    {
        const char* chunk_id = reinterpret_cast<const char*>(file_data.data() + offset);
        const uint32_t chunk_size = read_le32(reinterpret_cast<const unsigned char*>(file_data.data()) + offset + 4);

        offset += 8;
        if (strncmp(chunk_id, "EXIF", 4) == 0) 
        {
            if (offset + chunk_size <= file_data.size()) 
            {
                const std::string_view json_id = (search_api ? "prompt" : "workflow");
                const auto from = file_data.data() + offset;
                const auto to = file_data.data() + offset + chunk_size;
                const auto it = std::search(from, to, json_id.begin(), json_id.end(), icomparer);
                if (it != to)
                    return offset + std::distance(from, it);
            }
        }

        // WebP chunks are padded to even sizes
        offset += chunk_size + (chunk_size & 1);
    }

    return std::string::npos;
}

std::string extract_workflow_from_webp(const std::filesystem::path& webp_path)
{
    const auto file_data = tools::path::read_file(webp_path, false);
    if (file_data.empty())
        return {};

    auto pos = extract_webp_exif(file_data, false);
    if (std::string::npos == pos)
    {
        pos = extract_webp_exif(file_data, true);
        if (std::string::npos == pos)
            return {};
    }

    return cleanup_json(file_data, pos);
}

void scale(int32_t image_w, int32_t image_h, int32_t max_w, int32_t max_h, int32_t& res_w, int32_t& res_h)
{
    res_w = res_h = 0;
    if (0 == image_w || 0 == image_h || 0 == max_w || 0 == max_h)
        return;

    if (image_w <= max_w && image_h <= max_h)
    {
        res_w = image_w;
        res_h = image_h;
        return;
    }

    const float scale_w = cast::to_float(max_w) / cast::to_float(image_w);
    const float scale_h = cast::to_float(max_h) / cast::to_float(image_h);
    const float scale_factor = (scale_w < scale_h) ? scale_w : scale_h;

    const float res_wf = cast::to_float(image_w) * scale_factor;
    const float res_hf = cast::to_float(image_h) * scale_factor;

    res_w = cast::to_i32(std::max(res_wf, 1.0f));
    res_h = cast::to_i32(std::max(res_hf, 1.0f));
}

} // namespace img

}	//	namespace tools


#pragma once

namespace tools
{
namespace str
{

std::string& trim_this(std::string& str, const std::string& chars = " \r\n\t");
std::string trim_their(const std::string& str, const std::string& chars = " \r\n\t");

std::string& replace_this(std::string& str, const std::string& find, const std::string& replace, size_t from_pos = 0);
std::string replace_their(const std::string& str, const std::string& find, const std::string& replace, size_t from_pos = 0);

std::vector<std::string> split(const std::string& str, char separator);
std::string merge(const std::vector<std::string>& items, char separator);
std::string merge(const std::set<std::string>& items, char separator);

std::string to_lower_ascii(const std::string& str);

size_t hash_string(const std::string& str)  noexcept;
uint64_t hash_fnv1a(const void* data, size_t size) noexcept;
uint64_t hash_fnv1a(const char* data, size_t size) noexcept;
uint64_t hash_fnv1a(size_t data) noexcept;
uint64_t hash_fnv1a(const std::string& data) noexcept;
uint32_t crc32(const std::string& data) noexcept;

std::string generate_uuid_v4();

std::vector<byte> base64_decode(const std::string& base64);
std::vector<byte> base64_decode(const char* base64, size_t len);

std::string sha256(const std::vector<byte>& src);

#ifdef _WIN32
std::wstring utf8_to_wide(const std::string& utf8_str);
#endif

std::string u8_to_string(const std::u8string& u8_str);
std::string path_to_string(const std::filesystem::path& path);

std::string url_encode(const std::string& url);

} // namespace str

namespace path
{

bool exists(const std::filesystem::path& path) noexcept;

std::filesystem::path find_next_name(const std::filesystem::path& path);

std::optional<std::filesystem::path> copy(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename);
std::optional<std::filesystem::path> move(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename);

bool make_folder(const std::filesystem::path& dirpath);
bool make_file(const std::filesystem::path& filepath, bool overwrite, const std::vector<byte>& data_to_write);
bool make_file_path(const std::filesystem::path& filepath, bool overwrite, const std::vector<byte>& data_to_write);
bool make_file(const std::filesystem::path& filepath, bool overwrite, const std::string& data_to_write);
bool make_file_path(const std::filesystem::path& filepath, bool overwrite, const std::string& data_to_write);

std::string read_file(const std::filesystem::path& filepath, bool add_tail) noexcept;

bool remove(const std::filesystem::path& filepath) noexcept;
bool rename(const std::filesystem::path& from, const std::filesystem::path& to);

bool is_file(const std::filesystem::path& path) noexcept;
bool is_dir(const std::filesystem::path& path) noexcept;
bool is_empty(const std::filesystem::path& path) noexcept;

std::string get_extension(const std::filesystem::path& path);

// creates list of objects with pathes
std::vector<std::filesystem::path> enumerate_directory(const std::filesystem::path& path,
    bool recursively, bool include_dirs, bool include_files, bool relative_pathes) noexcept;

std::vector<std::string> enumerate_directory_set_slash(const std::filesystem::path& path,
    bool recursively, bool include_dirs, bool include_files, bool relative_pathes, char slash);

using FdSelectResultSg = std::function<void(bool, std::filesystem::path&&)>;
using FdSelectResultMt = std::function<void(bool, std::list<std::filesystem::path>&&)>;
void select_file_open(std::list<std::pair<std::wstring, std::wstring>>&& filters, const FdSelectResultSg& callback);
void select_files_open(std::list<std::pair<std::wstring, std::wstring>>&& filters, const FdSelectResultMt& callback);
void select_file_save(std::list<std::pair<std::wstring, std::wstring>>&& filters, std::filesystem::path init_file, const FdSelectResultSg& callback);
void select_dir(const FdSelectResultSg& callback);
void select_dirs(const FdSelectResultMt& callback);

#ifdef _WIN32
std::wstring build_commandline_arg(const std::wstring& arg);
std::string build_commandline_arg(const std::string& arg);
#endif

void open_file(const std::filesystem::path& path);

std::string& clear_filename(std::string& filename);

}	// namespace path

namespace api
{
std::vector<std::string> scan_for_disks();

} // namespace api

namespace img
{
std::string extract_workflow_from_png(const std::filesystem::path& png_path);
std::string extract_workflow_from_webp(const std::filesystem::path& webp_path);
void scale(int32_t image_w, int32_t image_h, int32_t max_w, int32_t max_h, int32_t& res_w, int32_t& res_h) noexcept;
std::string get_mime_type(const std::filesystem::path& filepath);
std::string get_readable_aspect_ratio(int32_t w, int32_t h);
}

};	// namespace tools

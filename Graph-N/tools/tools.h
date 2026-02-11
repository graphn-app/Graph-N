
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

size_t hash_string(const std::string& str);
uint64_t hash_fnv1a(const void* data, size_t size);
uint64_t hash_fnv1a(const char* data, size_t size);
uint64_t hash_fnv1a(size_t data);
uint64_t hash_fnv1a(const std::string& data);
uint32_t crc32(const std::string& data);

std::string generate_uuid_v4();

std::vector<byte> base64_decode(const std::string& base64);
std::vector<byte> base64_decode(const char* base64, size_t len);

std::string sha256(const std::vector<byte>& src);

} // namespace str

namespace path
{

bool exists(const std::filesystem::path& path);

std::filesystem::path find_next_name(const std::filesystem::path& path);

std::optional<std::filesystem::path> copy(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename);
std::optional<std::filesystem::path> move(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename);

bool make_folder(const std::filesystem::path& dirpath);
bool make_file(const std::filesystem::path& filepath, bool overwrite, const std::vector<byte>& data_to_write);
bool make_file_path(const std::filesystem::path& filepath, bool overwrite, const std::vector<byte>& data_to_write);
bool make_file(const std::filesystem::path& filepath, bool overwrite, const std::string& data_to_write);
bool make_file_path(const std::filesystem::path& filepath, bool overwrite, const std::string& data_to_write);

std::vector<char> read_file(const std::filesystem::path& filepath);

bool remove(const std::filesystem::path& filepath);
bool rename(const std::filesystem::path& from, const std::filesystem::path& to);

bool is_file(const std::filesystem::path& path);
bool is_dir(const std::filesystem::path& path);
bool is_empty(const std::filesystem::path& path);

// creates list of objects with pathes
std::vector<std::filesystem::path> enumerate_directory(const std::filesystem::path& path,
    bool recursively, bool include_dirs, bool include_files, bool relative_pathes);

std::vector<std::string> enumerate_directory_set_slash(const std::filesystem::path& path,
    bool recursively, bool include_dirs, bool include_files, bool relative_pathes, char slash);

using FdSelectResultSg = std::function<void(bool, std::filesystem::path&&)>;
using FdSelectResultMt = std::function<void(bool, std::list<std::filesystem::path>&&)>;
void select_file_open(std::list<std::pair<std::wstring, std::wstring>>&& filters, FdSelectResultSg& callback);
void select_files_open(std::list<std::pair<std::wstring, std::wstring>>&& filters, FdSelectResultMt& callback);
void select_file_save(std::list<std::pair<std::wstring, std::wstring>>&& filters, std::filesystem::path init_file, FdSelectResultSg& callback);
void select_dir(FdSelectResultSg& callback);
void select_dirs(FdSelectResultMt& callback);

}	// namespace path

namespace api
{
std::vector<std::string> scan_for_disks();

} // namespace api

};	// namespace tools

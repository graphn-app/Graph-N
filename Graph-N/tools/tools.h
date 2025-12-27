
#pragma once

namespace tools
{
namespace str
{

std::string trim(std::string string, std::string chars = " \r\n\t");
std::string replace(std::string string, const std::string& find, const std::string& replace, size_t from_pos = 0);
std::string format(const char* format, ...);
std::list<std::string> split(const std::string& str, char separator, bool add_empty);
std::string to_lower_ansi(const std::string& string);

size_t hash_string(const std::string& str);
size_t hash_fnv1a(const void* data, size_t size);
size_t hash_fnv1a(const char* data, size_t size);
size_t hash_fnv1a(size_t data);
size_t hash_fnv1a(const std::string& data);

} // namespace str

namespace path
{

bool exists(const std::filesystem::path& path);

std::string find_next_name(std::filesystem::path path);

// return: first - result of operation, second - dest name (might chnage if renamed)
std::optional<std::string> copy(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename);
std::optional<std::string> move(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite, bool rename);

bool make_folder(const std::filesystem::path& dirpath);
bool make_file(const std::filesystem::path& filepath, bool overwrite, const std::string& data_to_write = "");
bool make_file_path(const std::filesystem::path& filepath, bool overwrite);

std::vector<char> read_file(const std::filesystem::path& filepath);

bool remove(const std::filesystem::path& filepath);
bool rename(const std::filesystem::path& from, const std::filesystem::path& to);

bool is_file(const std::filesystem::path& path);
bool is_dir(const std::filesystem::path& path);
bool is_empty(const std::filesystem::path& path);

std::string& add_backslash(std::string& path);
std::string& remove_backslash(std::string& path);

// creates list of objects with pathes
std::list<std::filesystem::path> enumerate_directory(const std::filesystem::path& path, bool recursively, bool include_dirs, bool include_files, bool relative_pathes, char slash = '\\');

}	// namespace path
};	// namespace tools

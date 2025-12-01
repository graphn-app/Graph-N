#pragma once

namespace fs
{

std::filesystem::path get_this_app_filepath();
std::filesystem::path get_root_directory_path();
std::filesystem::path get_log_directory_path();
std::filesystem::path get_res_directory_path();
std::filesystem::path get_crash_dump_directory_path();

}
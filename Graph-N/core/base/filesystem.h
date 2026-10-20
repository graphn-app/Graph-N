/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace fs
{

std::filesystem::path get_this_app_filepath();
std::filesystem::path get_root_directory_path();
std::filesystem::path get_log_directory_path();
std::filesystem::path get_res_directory_path();
std::filesystem::path get_temp_directory_path();
std::filesystem::path get_temp_filepath(const std::string& ext);
std::filesystem::path get_backup_directory_path();
std::filesystem::path get_crash_dump_directory_path();
std::filesystem::path get_host_path();
std::filesystem::path get_host_webprofile_path(uint64_t host_id);
std::filesystem::path get_app_icon_path(bool png);
void get_license_paths(std::filesystem::path& license, std::filesystem::path& third_party, std::filesystem::path& privacy, std::filesystem::path& readme);

}

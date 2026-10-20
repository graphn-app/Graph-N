/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <string>
#include <type_traits>
#include <vector>

#include <windows.h>
#include <shellapi.h>
#include <wrl.h>
#include <WebView2.h>
#include <shlwapi.h>

#pragma warning(push, 0)
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fmt/xchar.h>
#include <fmt/format.h>
#include <fmt/compile.h>
#pragma warning(pop)

#define APP_COMFYUI			"ComfyUI"
#define APP_CAPTION         "Graph-N_Host"
#define APP_CAPTION_W       L"Graph-N_Host"

using byte = uint8_t;
using byte_buff = std::vector<byte>;

#ifdef _WIN32
#define ERROR_MESSAGE(message)       ::MessageBox(nullptr, message, APP_CAPTION, MB_ICONSTOP)
#elif defined __APPLE__
#error Not implemented
#elif defined __linux__
#error Not implemented
#endif

#include "tools/tools_datetime.h"
#include "core/base/blob.h"
#include "tools/cast.h"
#include "core/base/filesystem.h"
#include "tools/tools.h"
#include "tools/logging.h"
#include "tools/blob_serializer.h"
#include "tools/host_data_serializer.h"

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
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <windows.h>
#include <shellapi.h>
#include <wrl.h>
#include <WebView2.h>
#include <shlwapi.h>
#include <dwmapi.h>

#pragma warning(push, 0)
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fmt/xchar.h>
#include <fmt/format.h>
#include <fmt/compile.h>
#pragma warning(pop)

// suppress static analyzer warnings
#pragma warning(disable : 26429) // Symbol is never tested for nullness, it can be marked as not_null (f.23).
#pragma warning(disable : 26432) // If you define or delete any default operation in the type, define or delete them all (c.21).
#pragma warning(disable : 26446) // Prefer to use gsl::at() instead of unchecked subscript operator (bounds.4).
#pragma warning(disable : 26451) // Arithmetic overflow: Using operator '*' on a 4 byte value and then casting the result to a 8 byte value. Cast the value to the wider type before calling operator '*' to avoid overflow (io.2).
#pragma warning(disable : 26455) // Default constructor should not throw. Declare it 'noexcept' (f.6).
#pragma warning(disable : 26457) // (void) should not be used to ignore return values, use 'std::ignore =' instead (es.48).
#pragma warning(disable : 26475) // Do not use function style casts (es.49). Prefer 'Type{value}' over 'Type(value)'.
#pragma warning(disable : 26481) // Don't use pointer arithmetic. Use span instead (bounds.1).
#pragma warning(disable : 26482) // Only index into arrays using constant expressions (bounds.2).
#pragma warning(disable : 26485) // No array to pointer decay (bounds.3).
#pragma warning(disable : 26493) // Don't use C-style casts (type.4).
#pragma warning(disable : 26494) // Always initialize an object (type.5).
#pragma warning(disable : 26495) // Variable is uninitialized. Always initialize a member variable (type.6).
#pragma warning(disable : 26812) // The enum type is unscoped. Prefer 'enum class' over 'enum' (Enum.3).
#pragma warning(disable : 26813) // Use 'bitwise and' to check if a flag is set.

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
#include "core/base/version.h"
#include "tools/cast.h"
#include "core/base/filesystem.h"
#include "tools/tools.h"
#include "tools/logging.h"
#include "tools/blob_serializer.h"
#include "tools/host_data_serializer.h"

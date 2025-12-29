#pragma once

// stl
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <variant>
#include <vector>

// Windows
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

// 3rd party
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <nlohmann/json.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <httplib.h>
#include <stb_image.h>
#pragma warning(push, 0)
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/compile.h>
#pragma warning(pop)

// app global

#define NO_COPY_MOVE(ClassName) \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;

#define NO_COPY_MOVE_CTORS(ClassName) \
	ClassName() = default; \
	~ClassName() = default; \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;

#include "icons_lucide.h"
#include "core/base/types.h"
#include "tools/cast.h"
#include "core/base/id.h"
#include "core/base/filesystem.h"
#include "tools/tools.h"
#include "tools/tools_datetime.h"
#include "tools/logging.h"
#include "ui/imgui_ex.h"
#include "graph-n.h"

#define APP_CAPTION			"Graph-N"
#define APP_CAPTION_LOW		"graph-n"
#define APP_CAPTION_W		L"Graph-N"

namespace tt = tools::time;


// global identifiers
#define GLOBAL_ID_DATE_PICKER_ID_FIRST		0x1000000
#define GLOBAL_ID_DATE_PICKER_ID_NEXT		0x1000004
#define GLOBAL_ID_DATE_PICKER_ID_LAST		0x1000054

namespace comfyui
{
constexpr const char* object_info = "/object_info";
}

//#ifdef _DEBUG
//if ()
//{
//    __debugbreak();
//}
//#endif

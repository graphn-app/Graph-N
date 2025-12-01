#pragma once

// std
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <memory>
#include <functional>
#include <filesystem>
#include <fstream>
#include <optional>

// Windows
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#endif

// 3rd party
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_node_editor.h"
#include "nlohmann/json.hpp"
#include "SQLiteCpp/SQLiteCpp.h"
#include "httplib.h"
#include "stb_image.h"

// app global
#include "core/base/types.h"
#include "tools/cast.h"
#include "core/base/id.h"
#include "core/base/filesystem.h"
#include "tools/tools.h"
#include "tools/tools_datetime.h"
#include "tools/logging.h"

#include "graph-n.h"

#define APP_CAPTION			"Graph-N"
#define APP_CAPTION_LOW		"graph-n"
#define APP_CAPTION_W		L"Graph-N"

namespace ne = ax::NodeEditor;
namespace tt = tools::time;
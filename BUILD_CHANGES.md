# Build Changes — Linux / GNU x86_64

This document details every change made to port Graph-N from a Windows/MSVC-only
codebase to a native Linux (GNU/Clang) build and to get the `Graph-N` target to
compile successfully. Committed on branch `feature/linux-gnu-x86_64-build-details`
(commit `27c4746`).

---

## 1. High-level summary

| Area | What changed |
|------|--------------|
| `CMakeLists.txt` | Added a GNU/Clang toolchain block; made Windows-only sections conditional on `WIN32`; linked OpenGL + Threads on Linux; skipped the WebView2 host on non-Windows. |
| `Graph-N/main/main_linux.cpp` / `main_linux.h` | New Linux entry point and window/GLFW setup for the native GUI. |
| `Graph-N/graph-n.cpp` | Small Linux-specific tweaks (4 lines). |
| `Graph-N/pch.h` | Precompiled-header adjustments (11 lines). |
| Managers (`core/managers/*`) | Linux variants of ComfyUI, connect (HTTPS), and host managers. |
| Tools (`tools/*`) | Logging, host data serializer, and general tools updated. |
| UI helpers (`ui/helpers/*`) | Linux texture, video-control implementations. |
| Workflow / panels | `core/workflow/pin.cpp`, `ui/panels/panel_stats.cpp`. |
| Third-party blocker | Added missing `stb/stb_image_write.h` (see §4). |

Net result: `cmake --build /tmp/opencode/gn-build -j8 --target Graph-N` → **[100%] Built target Graph-N**.

---

## 2. `CMakeLists.txt` — the core of the port

The file previously assumed MSVC/Windows everywhere. The following changes make it
compiler- and platform-aware.

### 2.1 GNU/Clang compiler settings (new block)

Inserted after the existing `if(MSVC)` block:

```cmake
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GNU / Clang settings (Linux x86_64 is the primary target)
    find_package(Threads REQUIRED)
    add_compile_options(-Wall -Wextra -pthread)
    if(USE_ASAN)
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
endif()
```

- `find_package(Threads REQUIRED)` → provides `Threads::Threads`.
- `-Wall -Wextra -pthread` → standard warnings and pthread linkage.
- ASan support mirrors the MSVC path.

### 2.2 Google Test — guard the Windows-only CRT option

`gtest_force_shared_crt ON` is MSVC-specific, so it is now wrapped in `if(WIN32)`.

### 2.3 ImGui backend linking on Unix

The GLFW link went from unconditional `glfw` to a local-dev fallback:

```cmake
if(EXISTS "/tmp/opencode/deps/glfw-install/lib64/libglfw.so")
    target_link_libraries(ImGuiLib PRIVATE "/tmp/opencode/deps/glfw-install/lib64/libglfw.so")
    target_include_directories(ImGuiLib PRIVATE "/tmp/opencode/deps/glfw-install/include")
else()
    target_link_libraries(ImGuiLib PRIVATE glfw)
endif()
```

> ⚠️ The `/tmp/opencode/...` path is **environment-specific** (this machine has no system `glfw-devel`). On a normal system with `libglfw-dev` installed, the `else()` branch (`glfw`) is used.

### 2.4 Resources — `.rc` is Windows-only

`graph-n.rc` (Win32 resource script) is now only set on Windows:

```cmake
if(WIN32)
    set(GRAPHN_RESOURCES "Graph-N/res/graph-n.rc")
else()
    set(GRAPHN_RESOURCES "")
endif()
```

`add_executable(Graph-N ...)` dropped the `WIN32` GUI-subsystem keyword so Linux
builds a normal console/GLFW executable.

### 2.5 Graph-N include path (local GLFW)

Two duplicate blocks add `/tmp/opencode/deps/glfw-install/include` to `Graph-N`
include dirs when that local GLFW install exists (same env-specific workaround
as above).

### 2.6 Defines split by platform

Windows-only defines (`_CRT_SECURE_NO_WARNINGS`, `_CRT_NON_CONFORMING_SWPRINTFS`,
`_WIN32_WINNT=0x0A00`, `WIN32_LEAN_AND_MEAN`, `NOMINMAX`) moved inside
`if(WIN32)`. Cross-platform defines (`IMGUI_USE_WCHAR32`, `BOOST_ASIO_NO_LIB`)
stay unconditional.

### 2.7 Link libraries — platform split

**Windows** keeps the MSVC set (Shell32, wbemuuid, mfplat, ws2_32, crypt32,
d3d11, d3dcompiler, dwmapi).

**Linux/Unix** now links:
- `OpenGL::GL` (via `find_package(OpenGL REQUIRED)`)
- `Threads::Threads`
- `dl` (on Linux specifically)

The old duplicated `ws2_32 crypt32` / `d3d11 d3dcompiler dwmapi` blocks and the
old `glfw OpenGL::GL` Linux line were consolidated.

### 2.8 Host (WebView2) — Windows only

The entire `Graph-N_Host` target and its POST_BUILD copy commands are now wrapped
in `if(WIN32) ... endif()`. On Linux the host is skipped entirely (it depends on
WebView2, which is Windows-only).

### 2.9 Removed MSVC-only debugger properties

The `VS_DEBUGGER_WORKING_DIRECTORY` / `VS_DEBUGGER_COMMAND` `set_target_properties`
calls (Visual Studio F5 debug settings) were deleted since they are meaningless
outside MSVC.

---

## 3. Per-file changes (from `git diff --stat`)

| File | +/- | Role in the Linux port |
|------|-----|------------------------|
| `CMakeLists.txt` | 173 | Platform-conditional toolchain, linking, resources, host skip (§2). |
| `Graph-N/main/main_linux.cpp` | 226 | Linux `main()` entry point, GLFW window creation, ImGui GLFW/OpenGL3 backend init, main loop. |
| `Graph-N/main/main_linux.h` | 16 | Declarations for the Linux main/window helpers. |
| `Graph-N/graph-n.cpp` | 4 | Minor Linux-specific adjustments. |
| `Graph-N/pch.h` | 11 | Precompiled header tweaks for the Linux toolchain. |
| `Graph-N/core/managers/comfyui_manager_linux.cpp` | 30 | Linux ComfyUI manager implementation. |
| `Graph-N/core/managers/comfyui_manager_linux.h` | 7 | Header for the Linux ComfyUI manager. |
| `Graph-N/core/managers/connect_manager_https.cpp` | 7 | HTTPS connection manager adjustments. |
| `Graph-N/core/managers/host_manager_linux.cpp` | 17 | Linux host manager. |
| `Graph-N/core/managers/host_manager_linux.h` | 3 | Linux host manager header. |
| `Graph-N/core/workflow/pin.cpp` | 12 | Pin/workflow tweaks. |
| `Graph-N/tools/host_data_serializer.cpp` | 2 | Serialization tweak. |
| `Graph-N/tools/logging.cpp` | 34 | Linux logging implementation. |
| `Graph-N/tools/logging.h` | 12 | Logging header. |
| `Graph-N/tools/tools.cpp` | 81 | General tools (filesystem, etc.) Linux support. |
| `Graph-N/tools/tools.h` | 2 | Tools header cleanup. |
| `Graph-N/ui/helpers/textures_linux.cpp` | 52 | Linux texture loading (stb_image). |
| `Graph-N/ui/helpers/textures_linux.h` | 3 | Texture header. |
| `Graph-N/ui/helpers/video_control_linux.cpp` | 60 | Linux video control implementation. |
| `Graph-N/ui/helpers/video_control_linux.h` | 15 | Video control header. |
| `Graph-N/ui/panels/panel_stats.cpp` | 38 | Stats panel Linux adjustments. |

**Totals:** 21 files, +666 / −139 lines (from the original diff stat).

---

## 4. The build blocker — missing `stb_image_write.h`

The very first build attempt failed:

```
Graph-N/stb_impl.cpp:11:10: fatal error: stb_image_write.h: No such file or directory
```

`stb_impl.cpp` does:

```cpp
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image.h"
#include "deprecated/stb_image_resize2.h"
#include "stb_image_write.h"
```

`stb_image.h` and `deprecated/stb_image_resize2.h` were already present in
`stb/`, but `stb_image_write.h` was missing entirely.

**Fix:** copied the genuine public-domain header (v1.16, Sean Barrett) from the
local dependency cache at `/tmp/opencode/deps/glfw-3.4/deps/stb_image_write.h`
into `stb/stb_image_write.h`. No network access was required.

After this, the build completed cleanly: **[100%] Built target Graph-N**.

---

## 5. Third-party dependencies (gitignored)

These are **not** in the repository (listed in `.gitignore`) and must be present
in the repo root before configuring:

```
boost/          BOOST_ROOT for beast/asio headers
cpp-httplib/    HTTP client
fmt/            {fmt} formatting library
googletest/     Google Test (unit tests)
json/           nlohmann json
miniaudio/      audio
sqlite/         SQLite amalgamation (sqlite3.c / sqlite3.h)
SQLiteCpp/      C++ wrapper around sqlite3
stb/            stb_image, stb_image_write, stb_image_resize2
```

`imgui/` and `imgui-node-editor/` **are** vendored/tracked in the repo and do not
need to be cloned.

---

## 6. Build commands that succeeded

Configure (done previously, build dir already exists):

```bash
cmake -S . -B /tmp/opencode/gn-build -DCMAKE_BUILD_TYPE=Release
```

Build:

```bash
cmake --build /tmp/opencode/gn-build -j8 --target Graph-N
```

Result: **[100%] Built target Graph-N** (no errors, no fatal errors).

The `Graph-N` executable is output to `bin/Release/` and copied to the repo root.

---

## 7. Environment-specific caveats

1. **Local GLFW path** — `CMakeLists.txt` references
   `/tmp/opencode/deps/glfw-install/lib64/libglfw.so` and its include dir because
   this box lacks system `glfw-devel`. On a machine with `libglfw-dev` installed,
   the `else()` branch (`find_package`/link `glfw`) applies. Consider replacing
   the hard-coded path with `find_package(glfw3 REQUIRED)` for a cleaner portable
   build.

2. **`stb_image_write.h`** — copied from a local cache rather than the official
   `nothings/stb` repo. For a clean checkout, clone `https://github.com/nothings/stb.git`
   into `stb/` (or place `stb_image_write.h` there manually).

3. **Line endings** — Git warns `LF will be replaced by CRLF` on several Linux
   source files; this is a `.gitattributes`/checkout artifact and does not affect
   compilation.

---

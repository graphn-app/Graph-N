# ------------------------------------------------------------------------------
# Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.24)
project(Graph-N)

# ------------------------------------------------------------------------------
# ANALYZERS & SANITIZERS SETTINGS
# ------------------------------------------------------------------------------
option(ANALYZERS_ON "Enable MSVC and Clang-Tidy static analyzers" OFF)
option(USE_ASAN "Enable AddressSanitizer" ON)

# ------------------------------------------------------------------------------
# Global Settings
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(GRAPHN_STATS_API_KEY $ENV{GRAPHN_STATS_API_KEY})
if(NOT GRAPHN_STATS_API_KEY)
    set(GRAPHN_STATS_API_KEY "-")
endif()

if(MSVC)
    # remove default warning config
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

    # ASan is incompatible with Edit and Continue (/ZI). 
    # Replace /ZI with /Zi globally for debug builds.
    string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/ZI" "/Zi" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    
    add_compile_options(/MP8)
    
    # Disable incremental linking globally to silence LNK4300
    add_link_options(/INCREMENTAL:NO)

    # Force MSVC to ignore warnings and code analysis in SYSTEM headers
    add_compile_options(/external:W0 /analyze:external-)

    if(USE_ASAN)
    	add_compile_options(/fsanitize=address)
    endif()
endif()

# Define the working directory variable
set(WORK_DIR "${CMAKE_SOURCE_DIR}/work_dir")

# Ensure the work_dir exists
file(MAKE_DIRECTORY ${WORK_DIR})

# Output directories for standard builds (bin/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

# ------------------------------------------------------------------------------
# Google Test (Unit Tests Framework)
# ------------------------------------------------------------------------------
# Force shared CRT for Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(googletest)
enable_testing()

# ------------------------------------------------------------------------------
# SQLite & SQLiteCpp
# ------------------------------------------------------------------------------
# --- SQLite C Library ---
add_library(sqlite3 STATIC "sqlite/sqlite3.c")
target_include_directories(sqlite3 PUBLIC "sqlite")
target_compile_definitions(sqlite3 PRIVATE SQLITE_ENABLE_FTS5)

if(UNIX)
    target_link_libraries(sqlite3 PRIVATE m dl pthread)
endif()

# --- SQLiteCpp Wrapper ---
file(GLOB SQLITECPP_SOURCES "SQLiteCpp/src/*.cpp")
add_library(SQLiteCpp STATIC ${SQLITECPP_SOURCES})
target_include_directories(SQLiteCpp PUBLIC 
    "SQLiteCpp/include"
    "sqlite"
)
target_link_libraries(SQLiteCpp PUBLIC sqlite3)

# ------------------------------------------------------------------------------
# fmt
# ------------------------------------------------------------------------------

# remove test, docs, install
option(FMT_INSTALL "Generate the install target" OFF)
option(FMT_TEST "Build the test suite" OFF)
option(FMT_DOC "Build the documentation" OFF)

add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:fmt,INTERFACE_INCLUDE_DIRECTORIES>)

# ------------------------------------------------------------------------------
# ImGui (Modern 1.89.9)
# ------------------------------------------------------------------------------
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    $<$<CONFIG:Debug>:${IMGUI_DIR}/imgui_demo.cpp>
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

# ImGui backend
if(WIN32)
    # Windows: DX11 and Win32
    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
        ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    )
    add_definitions(-DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD) # suppress XInput
else()
    # Unix: GLFW and OpenGL3
    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
endif()

add_library(ImGuiLib STATIC ${IMGUI_SOURCES})

# Publicly define math operators so it propagates to NodeEditor and Graph-N
target_compile_definitions(ImGuiLib PUBLIC 
    IMGUI_DEFINE_MATH_OPERATORS
    IMGUI_USE_WCHAR32)

target_include_directories(ImGuiLib PUBLIC 
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends
)

if(MSVC)
	target_compile_options(ImGuiLib PRIVATE "/EHs-c-")
    target_compile_definitions(ImGuiLib PRIVATE _HAS_EXCEPTIONS=0)
else()
    target_compile_options(ImGuiLib PRIVATE "-fno-exceptions")
endif()

if(UNIX)
    target_link_libraries(ImGuiLib PRIVATE glfw)
endif()

# ------------------------------------------------------------------------------
# ImGui Node Editor (Thedmd)
# ------------------------------------------------------------------------------
set(NODE_EDITOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor")

set(NODE_EDITOR_SOURCES
    "${NODE_EDITOR_DIR}/imgui_node_editor.cpp"
    "${NODE_EDITOR_DIR}/imgui_node_editor_api.cpp"
    "${NODE_EDITOR_DIR}/imgui_canvas.cpp"
    "${NODE_EDITOR_DIR}/misc/imgui_node_editor.natvis"
)

add_library(NodeEditorLib STATIC ${NODE_EDITOR_SOURCES})

target_include_directories(NodeEditorLib PUBLIC 
    "${NODE_EDITOR_DIR}"
    "${IMGUI_DIR}"
)

target_link_libraries(NodeEditorLib PUBLIC ImGuiLib)

if(MSVC)
	target_compile_options(NodeEditorLib PRIVATE "/EHs-c-")
    target_compile_definitions(NodeEditorLib PRIVATE _HAS_EXCEPTIONS=0)
else()
    target_compile_options(NodeEditorLib PRIVATE "-fno-exceptions")
endif()

# ------------------------------------------------------------------------------
# boost - beast & asio
# ------------------------------------------------------------------------------
set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/boost")

# ------------------------------------------------------------------------------
# miniaudio
# ------------------------------------------------------------------------------
set(MINIAUDIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/miniaudio")

set(MINIAUDIO_SOURCES
    "${MINIAUDIO_DIR}/miniaudio.c"
)

add_library(MiniAudioLib STATIC ${MINIAUDIO_SOURCES})

target_include_directories(MiniAudioLib PUBLIC 
    "${MINIAUDIO_DIR}"
)

# ------------------------------------------------------------------------------
# RelWithDebugInfo
# ------------------------------------------------------------------------------

if(MSVC)
    # /INCREMENTAL:NO
    # /OPT:REF
    # /OPT:ICF    
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO " /INCREMENTAL:NO /OPT:REF /OPT:ICF")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO " /INCREMENTAL:NO /OPT:REF /OPT:ICF")
endif()

# ------------------------------------------------------------------------------
# Graph-N App
# ------------------------------------------------------------------------------
# Look for sources directly in Graph-N folder (no 'src' subfolder)
file(GLOB_RECURSE GRAPHN_SOURCES "Graph-N/*.cpp")
file(GLOB_RECURSE GRAPHN_HEADERS "Graph-N/*.h")

# resources
set(GRAPHN_RESOURCES "Graph-N/res/graph-n.rc")

# source tree for Visual Studio
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Graph-N" PREFIX "Source Files" FILES ${GRAPHN_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Graph-N" PREFIX "Header Files" FILES ${GRAPHN_HEADERS})

# app
add_executable(Graph-N WIN32 ${GRAPHN_SOURCES} ${GRAPHN_HEADERS} ${GRAPHN_RESOURCES})

if(MSVC)
    # enable /EHa, this will override /EHsc
	set_target_properties(Graph-N PROPERTIES CXX_STANDARD_EXCEPTIONS OFF)
    target_compile_options(Graph-N PRIVATE /EHa /bigobj)
endif()

# --- PRECOMPILED HEADERS ---
target_precompile_headers(Graph-N PRIVATE "Graph-N/pch.h")

if(MSVC)
    # Force main() entry point even for GUI subsystem (fixes linker error in Release)
    target_link_options(Graph-N PRIVATE "/ENTRY:mainCRTStartup")
    target_link_options(Graph-N PRIVATE "/IGNORE:4099")

    # STRICT MODE: Warnings as Errors (/WX), High Warning Level (/W4)
    target_compile_options(Graph-N PRIVATE /W4 /WX /utf-8)
endif()

# defines
target_compile_definitions(Graph-N PRIVATE 
    _CRT_SECURE_NO_WARNINGS
    _CRT_NON_CONFORMING_SWPRINTFS
    IMGUI_USE_WCHAR32
    BOOST_ASIO_NO_LIB
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
	GRAPHN_STATS_API_KEY="${GRAPHN_STATS_API_KEY}"
)

# --- Include Directories ---
# SYSTEM flag tells compiler to ignore warnings in these headers
target_include_directories(Graph-N SYSTEM PRIVATE
    "nlohmann/include"
    "stb"
    "cpp-httplib"
    "${BOOST_ROOT}"
    "${CMAKE_CURRENT_SOURCE_DIR}/SQLiteCpp/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/sqlite"
    "${NODE_EDITOR_DIR}"
    "${IMGUI_DIR}" 
    "${IMGUI_DIR}/backends"
    "${MINIAUDIO_DIR}"
)

target_include_directories(Graph-N PRIVATE "Graph-N")

# --- Linking ---
if(WIN32)
    target_link_libraries(Graph-N PRIVATE
        ImGuiLib
        NodeEditorLib
        SQLiteCpp
        sqlite3
        MiniAudioLib
        fmt::fmt
        Shell32
        wbemuuid
        mfplat mfreadwrite mfuuid propsys
    )
else()
    target_link_libraries(Graph-N PRIVATE
        ImGuiLib
        NodeEditorLib
        SQLiteCpp
        sqlite3
        MiniAudioLib
        opengl32
        fmt::fmt
    )
endif()

if(WIN32)
    # Link system network libs for httplib
    target_link_libraries(Graph-N PRIVATE ws2_32 crypt32)
    target_link_libraries(Graph-N PRIVATE d3d11 d3dcompiler dwmapi)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(Graph-N PRIVATE glfw OpenGL::GL)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(Graph-N PRIVATE dl pthread)
    endif()
endif()

# ------------------------------------------------------------------------------
# Host
# ------------------------------------------------------------------------------
file(GLOB_RECURSE HOST_SOURCES "Graph-N_Host/*.cpp")
file(GLOB_RECURSE HOST_HEADERS "Graph-N_Host/*.h")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Graph-N_Host" PREFIX "Source Files" FILES ${HOST_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Graph-N_Host" PREFIX "Header Files" FILES ${HOST_HEADERS})

set(HOST_SHARED_SOURCES
	"Graph-N/tools/cast.h"
	"Graph-N/tools/logging.h"
	"Graph-N/tools/logging.cpp"
	"Graph-N/tools/tools.h"
	"Graph-N/tools/tools.cpp"
	"Graph-N/core/base/filesystem.h"
	"Graph-N/core/base/filesystem.cpp"
	"Graph-N/tools/tools_datetime.h"
	"Graph-N/tools/tools_datetime.cpp"
    "Graph-N/core/base/blob.h"
    "Graph-N/tools/blob_serializer.cpp"
    "Graph-N/tools/blob_serializer.h"
    "Graph-N/tools/host_data_serializer.cpp"
    "Graph-N/tools/host_data_serializer.h"
)

list(APPEND HOST_SOURCES ${HOST_SHARED_SOURCES})

# resources
set(HOST_RESOURCES "Graph-N_Host/res/host.rc")

# app
add_executable(Graph-N_Host WIN32 ${HOST_HEADERS} ${HOST_SOURCES} ${HOST_RESOURCES})

if(MSVC)
    # Force main() entry point even for GUI subsystem (fixes linker error in Release)
    target_link_options(Graph-N_Host PRIVATE "/ENTRY:mainCRTStartup")

    # STRICT MODE: Warnings as Errors (/WX), High Warning Level (/W4)
    target_compile_options(Graph-N_Host PRIVATE /W4 /WX /utf-8)
endif()

# --- Include Directories ---
target_include_directories(Graph-N_Host SYSTEM PRIVATE
	"Graph-N"
    "WebView"
    "cpp-httplib"
    "nlohmann/include"
)

target_include_directories(Graph-N_Host PRIVATE "Graph-N_Host")

target_compile_definitions(Graph-N_Host PRIVATE 	
	_CRT_SECURE_NO_WARNINGS
    _CRT_NON_CONFORMING_SWPRINTFS
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
	TOOLS_DETACHED_STATE
	CLEAR_LOG_MODE
	GRAPHN_STATS_API_KEY="${GRAPHN_STATS_API_KEY}"
)

target_precompile_headers(Graph-N_Host PRIVATE "Graph-N_Host/pch.h")

# --- Linking ---
if(WIN32)
    target_link_libraries(Graph-N_Host PRIVATE
        fmt::fmt
        "${CMAKE_CURRENT_SOURCE_DIR}/Graph-N_Host/res/WebView2Loader.dll.lib"
        shlwapi
    )
endif()

# ------------------------------------------------------------------------------
# ANALYZERS SETTINGS
# ------------------------------------------------------------------------------

if(ANALYZERS_ON)
    # MSVC Code Analysis
    if(MSVC)
        set_target_properties(Graph-N PROPERTIES
            VS_GLOBAL_RunCodeAnalysis "true"
            VS_GLOBAL_CodeAnalysisRuleSet "CppCoreCheckRules.ruleset"
        )
        set_target_properties(Graph-N_Host PROPERTIES
            VS_GLOBAL_RunCodeAnalysis "true"
            VS_GLOBAL_CodeAnalysisRuleSet "CppCoreCheckRules.ruleset"
        )
    endif()

    # Clang-Tidy
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy" PATHS "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin")
    if (CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=bugprone-*,performance-*,modernize-*,readability-*")
    endif()
else()	
    if(MSVC)
        set_target_properties(Graph-N PROPERTIES
            VS_GLOBAL_RunCodeAnalysis "false"
            VS_GLOBAL_EnableMicrosoftCodeAnalysis "false"
        )
        set_target_properties(Graph-N_Host PROPERTIES
            VS_GLOBAL_RunCodeAnalysis "false"
            VS_GLOBAL_EnableMicrosoftCodeAnalysis "false"
        )
    endif()

    unset(CMAKE_CXX_CLANG_TIDY CACHE)
    set(CMAKE_CXX_CLANG_TIDY "")
endif()

# ------------------------------------------------------------------------------
# Debug Workflow Automation
# ------------------------------------------------------------------------------

# copying to work_dir
add_custom_command(TARGET Graph-N POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Graph-N> ${WORK_DIR}/$<TARGET_FILE_NAME:Graph-N>
    COMMENT "Copying Graph-N to work_dir..."
)

add_custom_command(TARGET Graph-N_Host POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Graph-N_Host> ${WORK_DIR}/$<TARGET_FILE_NAME:Graph-N_Host>
    COMMENT "Copying Graph-N_Host to work_dir..."
)

# debug run - Visual Studio (F5)
set_target_properties(Graph-N PROPERTIES 
    # run from work_dir
    VS_DEBUGGER_WORKING_DIRECTORY "${WORK_DIR}"
    
    # Debug -> run from work_dir/Graph-N.exe
    # Release -> run from bin/Graph-N.exe
    VS_DEBUGGER_COMMAND "$<$<CONFIG:Debug>:${WORK_DIR}/$<TARGET_FILE_NAME:Graph-N>>$<$<NOT:$<CONFIG:Debug>>:$<TARGET_FILE:Graph-N>>"
)
set_target_properties(Graph-N_Host PROPERTIES 
    # run from work_dir
    VS_DEBUGGER_WORKING_DIRECTORY "${WORK_DIR}"
    
    # Debug -> run from work_dir/Graph-N.exe
    # Release -> run from bin/Graph-N.exe
    VS_DEBUGGER_COMMAND "$<$<CONFIG:Debug>:${WORK_DIR}/$<TARGET_FILE_NAME:Graph-N_Host>>$<$<NOT:$<CONFIG:Debug>>:$<TARGET_FILE:Graph-N_Host>>"
)

# ------------------------------------------------------------------------------
# UNIT TESTS
# ------------------------------------------------------------------------------

# create unit tests file, if not exists
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp" 
    "#include <gtest/gtest.h>\n\nTEST(SanityCheck, TrueIsTrue) {\n    EXPECT_EQ(true, true);\n}\n")
endif()

file(GLOB TEST_ONLY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")

# list of files for testing (we take all app sources)
set(TEST_SOURCES ${GRAPHN_SOURCES})

# exclude ONLY the entry point (main/WinMain) so it doesn't conflict with gtest_main
list(REMOVE_ITEM TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Graph-N/entry.cpp")

# Combine test files, app sources (without entry.cpp), and headers
set(ALL_TEST_FILES ${TEST_ONLY_SOURCES} ${TEST_SOURCES} ${GRAPHN_HEADERS})

# Visual Studio source groups
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/tests" PREFIX "Test Files" FILES ${TEST_ONLY_SOURCES})

# make unit test app
add_executable(UnitTests ${ALL_TEST_FILES})

if(MSVC)
    target_compile_options(UnitTests PRIVATE /bigobj)
    
    # STRICT MODE: Warnings as Errors (/WX), High Warning Level (/W4)
    target_compile_options(UnitTests PRIVATE /W4 /WX /utf-8)
endif()

target_include_directories(UnitTests SYSTEM PRIVATE
    "nlohmann/include"
    "stb"
    "cpp-httplib"
    "${BOOST_ROOT}"
    "${CMAKE_CURRENT_SOURCE_DIR}/SQLiteCpp/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/sqlite"
    "${NODE_EDITOR_DIR}"
    "${IMGUI_DIR}" 
    "${IMGUI_DIR}/backends"
    "${MINIAUDIO_DIR}"
)

target_include_directories(UnitTests PRIVATE "Graph-N")

target_compile_definitions(UnitTests PRIVATE 
    _CRT_SECURE_NO_WARNINGS
    _CRT_NON_CONFORMING_SWPRINTFS
    IMGUI_USE_WCHAR32
    BOOST_ASIO_NO_LIB
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    SKIP_DEBUG_CATCH
)

target_precompile_headers(UnitTests PRIVATE "Graph-N/pch.h")

# Link your app's dependencies PLUS Google Test
target_link_libraries(UnitTests PRIVATE 
    ImGuiLib
    NodeEditorLib
    SQLiteCpp
    sqlite3
    MiniAudioLib
    fmt::fmt
    Shell32
    wbemuuid
    gtest
    gtest_main
)

if(WIN32)
    target_link_libraries(UnitTests PRIVATE 
        ws2_32 
        crypt32 
        d3d11 
        d3dcompiler 
        dwmapi
        mfplat mfreadwrite mfuuid propsys
    )
endif()

include(GoogleTest)
gtest_discover_tests(UnitTests)

# ------------------------------------------------------------------------------
# Debug Workflow Automation
# ------------------------------------------------------------------------------

# copying to work_dir
add_custom_command(TARGET UnitTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:UnitTests> ${WORK_DIR}/$<TARGET_FILE_NAME:UnitTests>
    COMMENT "Copying UnitTests to work_dir..."
)

# debug run - Visual Studio (F5)
set_target_properties(UnitTests PROPERTIES 
    # run from work_dir
    VS_DEBUGGER_WORKING_DIRECTORY "${WORK_DIR}"
    
    # Debug -> run from work_dir/UnitTests.exe
    # Release -> run from bin/UnitTests.exe
    VS_DEBUGGER_COMMAND "$<$<CONFIG:Debug>:${WORK_DIR}/$<TARGET_FILE_NAME:UnitTests>>$<$<NOT:$<CONFIG:Debug>>:$<TARGET_FILE:UnitTests>>"
)
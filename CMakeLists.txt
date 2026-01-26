cmake_minimum_required(VERSION 3.16)
project(Graph-N)

# ------------------------------------------------------------------------------
# Google Test (Unit Tests Framework)
# ------------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Force shared CRT for Windows (избегаем конфликтов runtime библиотек)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

# ------------------------------------------------------------------------------
# Global Settings
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Define the working directory variable
set(WORK_DIR "${CMAKE_SOURCE_DIR}/work_dir")

# Ensure the work_dir exists
file(MAKE_DIRECTORY ${WORK_DIR})

# Output directories for standard builds (bin/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

# ------------------------------------------------------------------------------
# SQLite & SQLiteCpp
# ------------------------------------------------------------------------------
# --- SQLite C Library ---
add_library(sqlite3 STATIC "sqlite/sqlite3.c")
target_include_directories(sqlite3 PUBLIC "sqlite")

# Suppress warnings for sqlite3
if(MSVC)
    target_compile_options(sqlite3 PRIVATE /W0)
else()
    target_compile_options(sqlite3 PRIVATE -w)	
endif()

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

# Suppress warnings for SQLiteCpp
if(MSVC)
    target_compile_options(SQLiteCpp PRIVATE /W0)
else()
    target_compile_options(SQLiteCpp PRIVATE -w)	
endif()

# ------------------------------------------------------------------------------
# fmt
# ------------------------------------------------------------------------------

# remove test, docs, install
option(FMT_INSTALL "Generate the install target" OFF)
option(FMT_TEST "Build the test suite" OFF)
option(FMT_DOC "Build the documentation" OFF)

add_subdirectory(fmt)

if(MSVC)
    target_compile_options(fmt PRIVATE /W0)
else()
    target_compile_options(fmt PRIVATE -w)
endif()

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
    # Windows: DX11 и Win32
    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
        ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    )
    add_definitions(-DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD) # suppress XInput
else()
    # Unix: GLFW и OpenGL3
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

if(UNIX)
    target_link_libraries(ImGuiLib PRIVATE glfw)
endif()

# Suppress warnings for ImGui
if(MSVC)
    target_compile_options(ImGuiLib PRIVATE /W0)
else()
    target_compile_options(ImGuiLib PRIVATE -w)	
endif()

# ------------------------------------------------------------------------------
# ImGui Node Editor (Thedmd)
# ------------------------------------------------------------------------------
set(NODE_EDITOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor")

set(NODE_EDITOR_SOURCES
    "${NODE_EDITOR_DIR}/imgui_node_editor.cpp"
    "${NODE_EDITOR_DIR}/imgui_node_editor_api.cpp"
    "${NODE_EDITOR_DIR}/crude_json.cpp"
    "${NODE_EDITOR_DIR}/imgui_canvas.cpp"
    "${NODE_EDITOR_DIR}/misc/imgui_node_editor.natvis"
	"${NODE_EDITOR_DIR}/misc/crude_json.natvis"
)

add_library(NodeEditorLib STATIC ${NODE_EDITOR_SOURCES})

target_include_directories(NodeEditorLib PUBLIC 
    "${NODE_EDITOR_DIR}"
    "${IMGUI_DIR}"
)

target_link_libraries(NodeEditorLib PUBLIC ImGuiLib)

# Suppress warnings for Node Editor
if(MSVC)
    target_compile_options(NodeEditorLib PRIVATE /W0)
else()
    target_compile_options(NodeEditorLib PRIVATE -w)	
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
    # /INCREMENTAL:NO
    # /OPT:REF
    # /OPT:ICF    
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO " /INCREMENTAL:NO /OPT:REF /OPT:ICF")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO " /INCREMENTAL:NO /OPT:REF /OPT:ICF")
endif()

if(MSVC)
    # enable /EHa, this will override /EHsc
    target_compile_options(Graph-N PRIVATE /EHa)
endif()

# --- PRECOMPILED HEADERS ---
target_precompile_headers(Graph-N PRIVATE "Graph-N/pch.h")

if(MSVC)
    # Force main() entry point even for GUI subsystem (fixes linker error in Release)
    target_link_options(Graph-N PRIVATE "/ENTRY:mainCRTStartup")

    # STRICT MODE: Warnings as Errors (/WX), High Warning Level (/W4)
    target_compile_options(Graph-N PRIVATE /W4 /WX /utf-8)
    
    # Optional: Disable very specific annoying warnings if needed, e.g.:
    # target_compile_options(Graph-N PRIVATE /wd4100) # Unreferenced formal parameter
endif()

# WebSocket++ Settings: Use Standalone Asio (No Boost)
target_compile_definitions(Graph-N PRIVATE 
    ASIO_STANDALONE 
    _WEBSOCKETPP_CPP11_STL_
    _CRT_SECURE_NO_WARNINGS
    _CRT_NON_CONFORMING_SWPRINTFS
    IMGUI_USE_WCHAR32
)

# --- Include Directories ---
# SYSTEM flag tells compiler to ignore warnings in these headers
target_include_directories(Graph-N SYSTEM PRIVATE
	"Graph-N"
    "nlohmann/include"
    "stb"
    "cpp-httplib"
    "websocketpp"
    "asio/asio/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/SQLiteCpp/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/sqlite"
    "${NODE_EDITOR_DIR}"
    "${IMGUI_DIR}" 
    "${IMGUI_DIR}/backends"
)

# Your own headers (checked for warnings)
target_include_directories(Graph-N PRIVATE "Graph-N")

# --- Linking ---
if(WIN32)
    target_link_libraries(Graph-N PRIVATE
        ImGuiLib
        NodeEditorLib
        SQLiteCpp
        sqlite3
		fmt::fmt
    )
else()
    target_link_libraries(Graph-N PRIVATE
        ImGuiLib
        NodeEditorLib
        SQLiteCpp
        sqlite3
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
# Debug Workflow Automation
# ------------------------------------------------------------------------------

# copying to work_dir
add_custom_command(TARGET Graph-N POST_BUILD
    COMMAND $<$<CONFIG:Debug>:${CMAKE_COMMAND}> 
            $<$<CONFIG:Debug>:-E> 
            $<$<CONFIG:Debug>:copy_if_different> 
            $<$<CONFIG:Debug>:$<TARGET_FILE:Graph-N>> 
            $<$<CONFIG:Debug>:${WORK_DIR}/$<TARGET_FILE_NAME:Graph-N>>
    COMMENT "Copying Debug executable to work_dir..."
)

# debug run - Visual Studio (F5)
set_target_properties(Graph-N PROPERTIES 
    # run from work_dir
    VS_DEBUGGER_WORKING_DIRECTORY "${WORK_DIR}"
    
    # Debug -> run from work_dir/Graph-N.exe
    # Release -> run from bin/Graph-N.exe
    VS_DEBUGGER_COMMAND "$<$<CONFIG:Debug>:${WORK_DIR}/$<TARGET_FILE_NAME:Graph-N>>$<$<NOT:$<CONFIG:Debug>>:$<TARGET_FILE:Graph-N>>"
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

# list if files for testing
set(TEST_SOURCES ${GRAPHN_SOURCES})
list(APPEND TEST_SOURCES ${TEST_ONLY_SOURCES})

# exclude entry.cpp
list(REMOVE_ITEM TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Graph-N/entry.cpp")

file(GLOB_RECURSE WIN32_MAIN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Graph-N/platform/win32/*.cpp")
list(REMOVE_ITEM TEST_SOURCES ${WIN32_MAIN_SOURCES})

set(ALL_TEST_FILES ${TEST_ONLY_SOURCES} ${TEST_SOURCES} ${GRAPHN_HEADERS})

# Visual Studio
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/tests" PREFIX "Test Files" FILES ${TEST_ONLY_SOURCES})

# make unit test app
add_executable(UnitTests ${ALL_TEST_FILES})

target_include_directories(UnitTests SYSTEM PRIVATE
    "Graph-N"
    "nlohmann/include"
    "stb"
    "cpp-httplib"
    "websocketpp"
    "asio/asio/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/SQLiteCpp/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/sqlite"
    "${NODE_EDITOR_DIR}"
    "${IMGUI_DIR}" 
    "${IMGUI_DIR}/backends"
)

target_precompile_headers(UnitTests PRIVATE "Graph-N/pch.h")

target_compile_definitions(UnitTests PRIVATE 
    ASIO_STANDALONE 
    _WEBSOCKETPP_CPP11_STL_
    _CRT_SECURE_NO_WARNINGS
	IMGUI_DEFINE_MATH_OPERATORS
)

# do not link imgui and DX11
target_link_libraries(UnitTests PRIVATE 
    GTest::gtest_main
    SQLiteCpp
    sqlite3
	ImGuiLib
	NodeEditorLib
)

if(WIN32)
    target_link_libraries(UnitTests PRIVATE 
        ws2_32 
        crypt32 
        d3d11 
        d3dcompiler 
        dwmapi
    )
endif()

include(GoogleTest)
gtest_discover_tests(UnitTests)

# ------------------------------------------------------------------------------
# Debug Workflow Automation
# ------------------------------------------------------------------------------

# copying to work_dir
add_custom_command(TARGET UnitTests POST_BUILD
    COMMAND $<$<CONFIG:Debug>:${CMAKE_COMMAND}> 
            $<$<CONFIG:Debug>:-E> 
            $<$<CONFIG:Debug>:copy_if_different> 
            $<$<CONFIG:Debug>:$<TARGET_FILE:UnitTests>> 
            $<$<CONFIG:Debug>:${WORK_DIR}/$<TARGET_FILE_NAME:UnitTests>>
    COMMENT "Copying Debug executable to work_dir..."
)

# debug run - Visual Studio (F5)
set_target_properties(UnitTests PROPERTIES 
    # run from work_dir
    VS_DEBUGGER_WORKING_DIRECTORY "${WORK_DIR}"
    
    # Debug -> run from work_dir/UnitTests.exe
    # Release -> run from bin/UnitTests.exe
    VS_DEBUGGER_COMMAND "$<$<CONFIG:Debug>:${WORK_DIR}/$<TARGET_FILE_NAME:UnitTests>>$<$<NOT:$<CONFIG:Debug>>:$<TARGET_FILE:UnitTests>>"
)
/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#ifdef __linux__
#include "main/main_linux.h"
#include "graph-n.h"
#include "main.h"
#include "core/base/version.h"
#include "tools/logging.h"

#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GL/gl.h>

#include <chrono>

namespace platform
{

bool MainRoutine_Linux::init()
{
    if (!glfwInit())
    {
        LogCriticalS("Failed to initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    constexpr int32_t initial_width = 1280;
    constexpr int32_t initial_height = 800;

    const std::string caption = fmt::format(FMT_STRING("{} {}"), APP_CAPTION, ver::ver_as_caption());

    m_window = glfwCreateWindow(initial_width, initial_height, caption.c_str(), nullptr, nullptr);
    if (nullptr == m_window)
    {
        LogCriticalS("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigViewportsNoDecoration = false;
    io.IniFilename = APP_CAPTION_LOW ".ini";
    io.LogFilename = nullptr;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // font and icons
    prepare_fonts();

    // window callbacks
    glfwSetWindowUserPointer(m_window, this);
    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
    {
        auto* routine = cast::ptrs<MainRoutine_Linux>(glfwGetWindowUserPointer(window));
        if (routine != nullptr && g::pGraphN != nullptr)
            g::pGraphN->handle_wnd_on_close();
    });
    glfwSetWindowIconifyCallback(m_window, [](GLFWwindow* window, int32_t iconified)
    {
        auto* routine = cast::ptrs<MainRoutine_Linux>(glfwGetWindowUserPointer(window));
        if (routine == nullptr || g::pGraphN == nullptr)
            return;

        if (iconified)
            g::pGraphN->handle_wnd_minimized();
        else
            g::pGraphN->handle_wnd_restored();
    });

    m_second_tick = std::chrono::steady_clock::now();
    m_seconds_elapsed = 0;
    return true;
}

int32_t MainRoutine_Linux::run(const flow_callback& run_cb, const cleanup_callback& cleanup_cb, std::atomic_bool& request_focus)
{
    LogCriticalIf(run_cb && cleanup_cb, "Empty callbacks");

    bool done = false;
    while (!done)
    {
        glfwPollEvents();

        if (glfwWindowShouldClose(m_window))
            done = true;

        if (!done)
        {
            cleanup_cb();   // cleanup last frame - prepare for the next one

            m_is_rendering = true;   // lock re-entrancy
            const auto start_tick = std::chrono::high_resolution_clock::now();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            const bool cont = run_cb(done);  // main flow

            // Rendering
            ImGui::Render();

            int32_t fb_w = 0;
            int32_t fb_h = 0;
            glfwGetFramebufferSize(m_window, &fb_w, &fb_h);
            glViewport(0, 0, fb_w, fb_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(m_window);
            }

            const auto end_tick = std::chrono::high_resolution_clock::now();
            m_frame_render_time = std::chrono::duration<double, std::milli>(end_tick - start_tick).count();

            // 1 second timer
            {
                const auto now = std::chrono::steady_clock::now();
                if (now - m_second_tick >= 1s)
                {
                    ++m_seconds_elapsed;
                    m_second_tick = now;
                    if (g::pGraphN != nullptr)
                        g::pGraphN->handle_1sec_timer(cast::to_u32(m_seconds_elapsed));
                }
            }

            glfwSwapBuffers(m_window);

            m_is_rendering = false;   // unlock

            if (request_focus.load(std::memory_order_relaxed))
            {
                glfwFocusWindow(m_window);
                request_focus.store(false, std::memory_order_relaxed);
            }

            if (!cont)
                done = true;
        }
    }

    return 0;
}

void MainRoutine_Linux::stop_and_cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_window != nullptr)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}

double MainRoutine_Linux::get_frame_render_time() const noexcept
{
    return m_frame_render_time;
}

int64_t MainRoutine_Linux::get_seconds_elapsed() const noexcept
{
    return m_seconds_elapsed;
}

bool MainRoutine_Linux::get_ini_config_exists() const
{
    const auto ini_path = fs::get_root_directory_path() / (APP_CAPTION_LOW ".ini");
    return std::filesystem::exists(ini_path);
}

void MainRoutine_Linux::wnd_restore()
{
    if (m_window != nullptr)
    {
        glfwRestoreWindow(m_window);
        glfwFocusWindow(m_window);
    }
}

void MainRoutine_Linux::wnd_minimize()
{
    if (m_window != nullptr)
        glfwIconifyWindow(m_window);
}

void MainRoutine_Linux::wnd_maximize()
{
    if (m_window != nullptr)
        glfwMaximizeWindow(m_window);
}

void MainRoutine_Linux::wnd_show()
{
    if (m_window != nullptr)
    {
        glfwShowWindow(m_window);
        glfwFocusWindow(m_window);
    }
}

void MainRoutine_Linux::wnd_hide()
{
    if (m_window != nullptr)
        glfwHideWindow(m_window);
}

void MainRoutine_Linux::wnd_toggle_fullscreen()
{
    if (m_window == nullptr)
        return;

    if (glfwGetWindowMonitor(m_window) != nullptr)
    {
        glfwSetWindowMonitor(m_window, nullptr, 100, 100, 1280, 800, 0);
    }
    else
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
}

} // namespace platform

#endif // #ifdef __linux__

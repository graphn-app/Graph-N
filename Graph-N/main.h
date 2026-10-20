/*
*	Copyright (c) 2026 Serhii Kalenchuk. Licensed under the GPLv3. See LICENSE file.
*/

#pragma once

namespace global
{

using DragAndDropCallback = std::function<void(std::list<std::filesystem::path>&&)>;

class MainRoutine
{
public:
    MainRoutine() noexcept = default;
    virtual ~MainRoutine() noexcept = 0;

    NO_COPY_MOVE(MainRoutine);

    virtual bool init() = 0;
    virtual void stop_and_cleanup() = 0;

    void set_vsync_enabled(bool enable) noexcept { m_vsync = enable; }
    bool get_vsync_enabled() noexcept { return m_vsync; }

    virtual double get_frame_render_time() const noexcept = 0;
    virtual int64_t get_seconds_elapsed() const noexcept = 0;

    using cleanup_callback = std::function<void()>;
    using flow_callback = std::function<bool(bool DoExit)>;
    virtual int32_t run(const flow_callback& run_cb, const cleanup_callback& cleanup_cb, std::atomic_bool& request_focus) = 0;

    void set_dnd_callback(const DragAndDropCallback& callback) { m_dnd_callback = callback; }

    ImFont* get_regular_font() const noexcept { return m_font_regular; }
    ImFont* get_small_font() const noexcept { return m_font_small; }
    ImFont* get_bold_font() const noexcept { return m_font_bold; }
    ImFont* get_icons_font() const noexcept { return m_font_icons; }
    ImFont* get_big_icons_font() const noexcept { return m_font_big_icons; }

    virtual void wnd_restore() = 0;
    virtual void wnd_minimize() = 0;
    virtual void wnd_maximize() = 0;
    virtual void wnd_show() = 0;
    virtual void wnd_hide() = 0;

    virtual void wnd_toggle_fullscreen() = 0;
    bool get_is_fullscreen() const noexcept { return m_fullscreen; }

    virtual bool get_ini_config_exists() const = 0;

    uint32_t get_monitor_frequency() const noexcept { return m_monitor_frequency; }
    uint32_t& get_monitor_x() noexcept { return m_monitor_x; }
    uint32_t& get_monitor_y() noexcept { return m_monitor_y; }
    float& get_monitor_dpi() noexcept { return m_monitor_dpi; }

    float get_frame_budget() const noexcept { return m_frame_budget; }
    float get_frame_budget_warn() const noexcept { return m_frame_budget / 2.0f; }
    float get_frame_budget_crit() const noexcept { return m_frame_budget - 2.0f; }

protected:
    void prepare_fonts();

protected:
    DragAndDropCallback m_dnd_callback = nullptr;
    bool m_fullscreen = false;

    float m_frame_budget = 0.0f;
    uint32_t m_monitor_frequency = 0;
    uint32_t m_monitor_x = 0;
    uint32_t m_monitor_y = 0;
    float m_monitor_dpi = 0.0f;

private:
    bool m_vsync = true;
    ImFont* m_font_regular = nullptr;
    ImFont* m_font_small = nullptr;
    ImFont* m_font_bold = nullptr;
    ImFont* m_font_icons = nullptr;
    ImFont* m_font_big_icons = nullptr;
};

} // namespace global

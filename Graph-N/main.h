
#pragma once

namespace platform
{

using DragAndDropCallback = std::function<void(std::list<std::filesystem::path>&&)>;

class MainRoutine
{
public:
    MainRoutine() = default;
    virtual ~MainRoutine() = 0;

    NO_COPY_MOVE(MainRoutine);

    virtual bool init() = 0;
    virtual void free() = 0;

    inline void set_vsync_enabled(bool enable) { m_vsync = enable; }
    inline bool get_vsync_enabled() { return m_vsync; }

    virtual double get_frame_render_time() const = 0;

    using flow_callback = std::function<bool(bool DoExit)>;
    virtual int32_t run(flow_callback& callback, std::atomic_bool& request_focus) = 0;

    void set_dnd_callback(const DragAndDropCallback& callback) { m_dnd_callback = callback; }

    ImFont* get_regular_font() const { return m_font_regular; }
    ImFont* get_small_font() const { return m_font_small; }
    ImFont* get_bold_font() const { return m_font_bold; }
    ImFont* get_icons_font() const { return m_font_icons; }
    ImFont* get_big_icons_font() const { return m_font_big_icons; }

    virtual void wnd_restore() = 0;
    virtual void wnd_minimize() = 0;
    virtual void wnd_maximize() = 0;
    virtual void wnd_show() = 0;
    virtual void wnd_hide() = 0;

    virtual bool get_ini_config_exists() const = 0;

protected:
    void prepare_fonts();

protected:
    DragAndDropCallback m_dnd_callback = nullptr;

private:
    bool m_vsync = true;
    ImFont* m_font_regular = nullptr;
    ImFont* m_font_small = nullptr;
    ImFont* m_font_bold = nullptr;
    ImFont* m_font_icons = nullptr;
    ImFont* m_font_big_icons = nullptr;
};

} // namespace platform


#pragma once

namespace platform
{

class MainRoutine
{
public:
	MainRoutine() = default;
	virtual ~MainRoutine() = 0 {};

	virtual bool init() = 0;
	virtual void free() = 0;

	using flow_callback = std::function<bool(bool do_exit)>;
	virtual int run(flow_callback& callback, std::atomic_bool& request_focus) = 0;

    ImFont* get_regular_font() const { return m_font_regular; }
    ImFont* get_bold_font() const { return m_font_bold; }
    ImFont* get_icons_font() const { return m_font_icons; }

	virtual void wnd_restore() = 0;
	virtual void wnd_minimize() = 0;
	virtual void wnd_maximize() = 0;
	virtual void wnd_show() = 0;
	virtual void wnd_hide() = 0;

protected:
    void prepare_fonts();

private:
    ImFont* m_font_regular = nullptr;
    ImFont* m_font_bold = nullptr;
    ImFont* m_font_icons = nullptr;
};

} // namespace platform

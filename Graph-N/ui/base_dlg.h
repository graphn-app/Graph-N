#pragma once

// TODO - remove commented

namespace ui
{

struct DlgData
{
	float x = 0;
	float y = 0;
	float width = 0;
	float height = 0;

	std::string title;

	bool set_focus = true;
	//gid set_focus_id = 0;

	//std::map<gid, std::string> buffer;
	//std::map<gid, tt::dt> date_buffer;
	//GidSet failed_ids;
};

template <typename DlgDataType>
class BaseDlg
{
public:
	BaseDlg(DlgObject obj)
		: m_obj(obj)
	{}
	virtual ~BaseDlg() = 0 {}

	void set_focused() { m_focused = true; }

	DlgCode show_dialog(bool& focused)
	{
		if (m_close_called)
			return DlgCode::cancel;
		return show(focused);
	}
	void close_dialog()
	{
		m_close_called = true;
	}

	virtual void stop_all_threads() {}

protected:
	virtual DlgCode show(bool& focused) = 0;
	DlgDataType* data() { return m_data.get(); }

	virtual void init()
	{
		if (m_dlg_initialized)
			return;
		m_data = std::make_unique<DlgDataType>();
		m_dlg_initialized = true;
	}

	//void update_window_pos()
	//{
	//	const auto pos = ImGui::GetWindowPos();
	//	m_data->x = pos.x;
	//	m_data->y = pos.y;
	//}

	DlgObject get_dlg_obj() const { return m_obj; }

	//void set_exit_code(DlgCode code) { m_exit_code = code; }
	//void set_exit_code_ok() { m_exit_code = DlgCode::ok; }
	//DlgCode exit_code() const { return m_exit_code; }

	DlgCode cleanup_and_exit(bool end_dlg, bool pop_styles, DlgCode exit_code)
	{
		if (end_dlg)
			ImGui::End();

		if (pop_styles)
			ImGui::PopDlgTitleStyles();

		return exit_code;
	}
	//DlgCode cleanup_and_exit_popup(bool end_dlg, bool pop_styles, DlgCode exit_code);

	void set_focus()
	{
		if (m_focused)
			ImGui::SetNextWindowFocus();
		m_focused = false;
	}

private:
	DlgObject m_obj;
	DlgCode m_exit_code = DlgCode::progress;
	bool m_dlg_initialized = false;
	bool m_focused = false;
	//bool m_close_called = false;
	std::unique_ptr<DlgDataType> m_data;
};

} // namespace ui


#pragma once
#include "ui/base_dlg.h"

namespace ui
{

struct PanelCommonData : public DlgData
{
    const float button_width = 30.0f;
    const float grouped_btn_spacing_x = 1.0f;
    const float ungrouped_btn_spacing_x = 2.0f;
};

template <typename DataType>
class PanelBase : public BaseDlgWithData<DataType>
{
public:
    PanelBase(VisibleObject obj, const char* name, const char* id)
        : BaseDlgWithData(obj)
    {
        m_panel_name = tools::str::format("%s##%s", name, id);
    }

    ~PanelBase() override = default;

    const char* get_panel_name() const { return m_panel_name.c_str(); }

protected:
    bool begin_panel(bool& focused, ImU32 bg_color = IM_COL32_BLACK)
    {
        // m_class.ClassId = cast::to_u32(data()->class_id);
        // ImGui::SetNextWindowClass(&m_class);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);
        bool opened = true;
        focused = false;
        ImGui::Begin(m_panel_name.c_str(), &opened);
        if (opened)
            focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
        return opened;
    }

    DlgCode end_panel(bool opened)
    {
        ImGui::End();
        ImGui::PopStyleColor();
        return opened ? DlgCode::progress : DlgCode::cancel;
    }

private:
    std::string m_panel_name;

  //  ImGuiWindowClass m_class;
};


} // namespace ui

#pragma once
#include "ui/base_dlg.h"

namespace ui
{

struct PanelData final : public DlgData
{
    std::set<uint32_t> open_items;

    ImU32 clr_hover_group = 0;			// PanelNodes group frame
    ImU32 clr_hover_node = 0;			// PanelNodes node frame
};

class PanelBase : public BaseDlgWithData<PanelData>
{
public:
    PanelBase(VisibleObject obj, const char* name, const char* id);
    virtual ~PanelBase() = 0 {};

    const char* get_panel_name() const { return m_panel_name.c_str(); }

protected:
    bool begin_panel(bool& focused, ImU32 bg_color = IM_COL32_BLACK);
    DlgCode end_panel(bool opened);

private:
    std::string m_panel_name;

  //  ImGuiWindowClass m_class;
};


} // namespace ui
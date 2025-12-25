
#pragma once
#include "ui/base_dlg.h"

namespace ui
{

enum class PanelClassID
{
	_none,
	left,
	right,
	bottom,
	workflow
};

struct PanelData final : public DlgData
{
    PanelClassID class_id = PanelClassID::_none;
};

class PanelBase : public BaseDlgWithData<PanelData>
{
    using BaseDlgWithData<PanelData>::BaseDlgWithData;

public:
    virtual ~PanelBase() = 0 {};

protected:
    bool begin_panel(const char* name, bool& focused, ImU32 bg_color = IM_COL32_BLACK);
    DlgCode end_panel(bool opened);

private:
    ImGuiWindowClass m_class;
};


} // namespace ui
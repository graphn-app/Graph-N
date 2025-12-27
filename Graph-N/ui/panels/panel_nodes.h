
#pragma once
#include "ui/panels/panel_base.h"

namespace ui
{

struct PanelData final : public PanelCommonData
{
    enum Buffer
    {
		buff_filter,
        buff_filter_lowered,
        buff_count
    };

    std::set<hid> open_items;

    ImU32 clr_hover_group = 0;			// hovered group frame
    ImU32 clr_hover_node = 0;			// hovered node frame
    ImU32 clr_hover_btn = 0;			// hovered button
    ImU32 clr_active_btn = 0;			// active button

    bool tree_view = true;
};

class PanelNodes final : public PanelBase<PanelData>
{
public:
    PanelNodes();
    ~PanelNodes() override = default;

    void init() override;

private:
    DlgCode show(bool& focused) override;
    void show_tree_view();
    void show_plain_view();
	
    void show_cfg_buttons();
    void expand_all();
    void collapse_all();
};


} // namespace ui
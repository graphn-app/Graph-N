
#pragma once
#include "ui/panels/panel_base.h"

namespace data
{
class Workflow;
}

namespace ui
{

struct PanelWfListData final : public PanelCommonData
{
    ImU32 clr_active = 0;
    ImU32 clr_hover_btn = 0;
    ImU32 clr_active_btn = 0;
};

class PanelWfList final :
    public PanelEmptyWithDescr,
    public PanelWithList,
    public PanelToolbar,
    public PanelBase<PanelWfListData>
{
public:
    PanelWfList();
    ~PanelWfList() override;

    void init() override;
    void free() override;
    void stop_all_threads() override;

    void reload();
    void rebuild();

private:
    DlgCode show(bool& focused) override;
    void show_wfs();

    void setup_button_state_active();
    void setup_button_state_deleted();
    void setup_button_state_labels();

    void handle_dirty(const PanelItemState& dirty_item);

    void on_add_new_wf(PanelToolbar::Button& button);
    void on_rename_wf(PanelToolbar::Button& button);
    void on_pre_rename_wf(PanelToolbar::Button& button);
    void on_locate_wf(PanelToolbar::Button& button);
    void on_pre_locate_wf(PanelToolbar::Button& button);
    void on_delete_wf(PanelToolbar::Button& button);
    void on_pre_delete_wf(PanelToolbar::Button& button);
    void on_view_deleted(PanelToolbar::Button& button);
    void on_manage_label(PanelToolbar::Button& button);
    void on_add_label(PanelToolbar::Button& button);

private:
    std::vector<WfInfo> m_wfs;
    std::vector<WfInfo> m_deleted;
    std::vector<std::string> m_labels;
    std::vector<PanelListItem> m_panel_items;
    std::vector<PanelListItem> m_panel_items_deleted;
    std::vector<PanelListItem> m_panel_items_labels;
    wid m_active = 0;
    bool m_deleted_view = false;
    bool m_labels_view = false;
    bool m_new_label_state = false;
    int64_t m_renaming_wf = -1;
};


} // namespace ui
